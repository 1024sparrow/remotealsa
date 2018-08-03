#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>

#include <string>
#include <vector>

static int capture_buffer_frames = 128;
static snd_pcm_t* capture_handle = NULL;
static uint32_t capture_sample_rate = 16000;
static int capture_num_channels = 1;
static snd_output_t* alsa_log = NULL;

static std::vector<uint8_t> capture_buffer;
static snd_pcm_t* playback_handle = NULL;
static uint32_t playback_sample_rate = 16000;
static int playback_num_channels = 1;
static std::vector<uint8_t> playback_buffer;
static int playback_buffer_size;
static int playback_buffer_read;
static snd_pcm_uframes_t playback_frames;

#define D(FUNC) if ((err = FUNC) < 0) {\
    printf("[%s:%d] -- %s (%d):%s\n", __FILE__, (__LINE__ ), #FUNC, err, snd_strerror(err)); \
    exit(1);\
  } else { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    printf("%ld.%04ld LOG:(%04d) -- ", tv.tv_sec, tv.tv_usec, __LINE__);\
    printf("%s : ok\n", #FUNC);\
  }

#define LOG(FORMAT, ...) \
  do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    printf("%ld.%04ld LOG:(%04d) -- ", tv.tv_sec, tv.tv_usec, __LINE__); \
    printf(FORMAT, ##__VA_ARGS__); \
    printf("\n"); \
  } while (0)

static void setup_capture(char const* capture_handle_name)
{
  int err;
  uint32_t desired_sample_rate;
  snd_pcm_hw_params_t* params;
  snd_pcm_format_t fmt = SND_PCM_FORMAT_S16_LE;

  LOG("setup_capture with device:%s", capture_handle_name);

  D( snd_pcm_open(&capture_handle, capture_handle_name, SND_PCM_STREAM_CAPTURE, 0) );
  D( snd_pcm_hw_params_malloc(&params) );
  D( snd_pcm_hw_params_any(capture_handle, params) );
  D( snd_pcm_hw_params_set_access(capture_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) );
  D( snd_pcm_hw_params_set_format(capture_handle, params, fmt) );

  desired_sample_rate = capture_sample_rate;
  D( snd_pcm_hw_params_set_rate_near(capture_handle, params, &capture_sample_rate, 0));
  LOG("sampe_rate requested:%u configured:%u", desired_sample_rate, capture_sample_rate);

  D( snd_pcm_hw_params_set_channels(capture_handle, params, capture_num_channels));
  D( snd_pcm_hw_params(capture_handle, params) );
  
  snd_pcm_hw_params_free(params);

  D( snd_pcm_prepare(capture_handle) );

  const uint32_t n = (capture_buffer_frames * (snd_pcm_format_width(fmt) / 8) * capture_num_channels);
  capture_buffer.reserve(n);
  capture_buffer.resize(n);

  snd_pcm_dump(capture_handle, alsa_log);
}

static void setup_playback(char const* playback_handle_name)
{
  int err;
  uint32_t period_time;
  snd_pcm_hw_params_t* params;
  snd_pcm_uframes_t playback_frames = 16;

  LOG("setup_playback with device:%s", playback_handle_name);

  D( snd_pcm_open(&playback_handle, playback_handle_name, SND_PCM_STREAM_PLAYBACK, 0) );
  snd_pcm_hw_params_alloca(&params);
  D( snd_pcm_hw_params_any(playback_handle, params) );
  D( snd_pcm_hw_params_set_access(playback_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED));
//  D( snd_pcm_hw_params_get_period_size(params, &playback_frames, 0) );
  D( snd_pcm_hw_params_set_format(playback_handle, params, SND_PCM_FORMAT_S16_LE) );
  D( snd_pcm_hw_params_set_channels(playback_handle, params, playback_num_channels) );
  D( snd_pcm_hw_params_set_rate_near(playback_handle, params, &playback_sample_rate, 0) );
  D( snd_pcm_hw_params(playback_handle, params) );

  const uint32_t n = (playback_frames * playback_num_channels * 2);
  playback_buffer.reserve(n);
  playback_buffer.resize(n);
  playback_buffer_size = n;

  snd_pcm_dump(playback_handle, alsa_log);
}

static void timeval_subtract(timeval const& a, timeval const& b, timeval* result)
{
  LOG("%ld.%ld - %ld.%ld", a.tv_sec, a.tv_usec, b.tv_sec, b.tv_usec);

  result->tv_sec = a.tv_sec - b.tv_sec;
  result->tv_usec = a.tv_usec - b.tv_usec;
  if (result->tv_usec < 0)
  {
    result->tv_sec -= 1;
    result->tv_usec += 1000000;
  }
}

static void exception_handler(snd_pcm_t* h)
{
  int err;
  snd_pcm_status_t* status;
  snd_pcm_state_t state;

  snd_pcm_status_alloca(&status);
  D( snd_pcm_status(h, status) );

  snd_pcm_status_dump(status, alsa_log);

  state = snd_pcm_status_get_state(status);
  LOG("read/write error state:%s", snd_pcm_state_name(state));

  switch (state)
  {
    case SND_PCM_STATE_XRUN:
    {
      snd_timestamp_t now;
      snd_timestamp_t diff;
      snd_timestamp_t tstamp;

      snd_pcm_status_get_tstamp(status, &now);
      snd_pcm_status_get_trigger_tstamp(status, &tstamp);
      timeval_subtract(now, tstamp, &diff);
      LOG("overrune. (at least %0.3fms long)", (diff.tv_sec * 1000 + diff.tv_usec / 1000.0f));

      D( snd_pcm_prepare(h) );
      return;
    }
    break;

    case SND_PCM_STATE_DRAINING:
    {
      D( snd_pcm_prepare(h) );
      return;
    }
    break;
  }

  LOG("exiting");
  exit(1);
}

static void print_help()
{
  printf("\n");
  printf("\tUsage xaudio [OPTIONS]\n");
  printf("\t\t--port=<port>                     The port to listen on\n");
  printf("\t\t--capture=<devname>     -c <name> The ALSA capture device. If unsure, use 'default'\n");
  printf("\t\t--capture-channels=<n>  -d <n>    The number of channels. Use 1\n");
  printf("\t\t--capture-rate=<KHZ>    -r <KHZ>  The capture rate in hertz. Use 16000\n");
  printf("\t\t--capture-frames=<n>    -f <n>    Not sure, skip it.\n");
  printf("\t\t--playback=<devnam>     -p <name> The playback device name. If unsure, use 'default'\n");
  printf("\t\t--help                  -h        Print this help and exit\n");
  printf("\n");
  printf("Examples:\n");
  printf("\txaudio --port=10100 --capture=default\n");
  printf("\txaudio --port=10100 --capture=default --playback=default\n");
  printf("\n");
}


int main(int argc, char* argv[])
{
  int err = -1;
  int server_fd = -1;
  int port = -1;
  char const* capture_device = NULL;
  char const* playback_device = NULL;
  struct sockaddr_in server_addr = {0};


  struct option long_options[] =
  {
    { "port", required_argument, NULL, 10000 },

    { "capture", required_argument, NULL, 'c' },
    { "capture-channels", required_argument, NULL, 'd' },
    { "capture-rate", required_argument, NULL, 'r' },
    { "capture-frames", required_argument, NULL, 'f' },

    { "playback", required_argument, NULL, 'p' },
    { "help", no_argument, NULL, 'h' },
    { 0, 0, 0, 0 }
  };

  int c;
  while ((c = getopt_long(argc, argv, "f:c:p:d:r:h", long_options, NULL)) != -1)
  {
    switch (c)
    {
      case 'h':
        print_help();
        exit(0);
        break;
      case 'f':
        capture_buffer_frames = static_cast<int>(strtol(optarg, NULL, 10));
        break;
      case 'c':
        capture_device = optarg;
        break;
      case 'd':
        capture_num_channels = static_cast<int>(strtol(optarg, NULL, 10));
        break;
      case 'r':
        capture_sample_rate = static_cast<int>(strtol(optarg, NULL, 10));
        break;
      case 'p':
        playback_device = optarg;
        break;
      case 10000:
        port = static_cast<int>(strtol(optarg, NULL, 10));
        break;
      case '?':
        print_help();
        exit(0);
        break;
    }
  }

  if (port == -1)
  {
    printf("failed to provide listening port with --port=<port>\n");
    print_help();
    exit(0);
  }

  LOG("sound library:%s", snd_asoundlib_version());
  snd_output_stdio_attach(&alsa_log, stdout, 0);

  if (capture_device)
    setup_capture(capture_device);
  else
    LOG("skipping capture, no device supplied with -c");

  if (playback_device)
    setup_playback(playback_device);
  else
    LOG("skipping playback, no device supplied with  -p");

  LOG("capture_buffer_frames:%d", capture_buffer_frames);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  int enable = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

  err = bind(server_fd, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr));
  if (err < 0)
  {
    LOG("failed to bind listen socket. %s", strerror(errno));
    exit(1);
  }

  listen(server_fd, 2);
  LOG("listening for incoming connetions on:[%s:%d]",  inet_ntoa(server_addr.sin_addr), port);

  while (true)
  {
    struct sockaddr_in client_addr;
    socklen_t client_addr_length = sizeof(struct sockaddr);

    int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&client_addr),
      &client_addr_length);

    if (client_fd < 0)
    {
      LOG("error accepting client connection. %s", strerror(errno));
      continue;
    }

    LOG("accepted client connection from:[%s:%d]", inet_ntoa(client_addr.sin_addr),
      ntohs(client_addr.sin_port));

    while (true)
    {
      fd_set read_fds;
      fd_set write_fds;
      fd_set err_fds;
      FD_ZERO(&write_fds);
      FD_ZERO(&read_fds);
      FD_ZERO(&err_fds);

      // std::fill(capture_buffer.begin(), capture_buffer.end(), 0);
      if (capture_handle)
      {
        err = snd_pcm_readi(capture_handle, &capture_buffer[0], capture_buffer_frames);
        if (err != capture_buffer_frames)
        {
          exception_handler(capture_handle);
        }

        // only care about writing if we're in capture mode, therefore, sending 
        FD_SET(client_fd, &write_fds);
      }

      FD_SET(client_fd, &read_fds);
      FD_SET(client_fd, &err_fds);

      struct timeval timeout;
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;

      int ret = select(client_fd + 1, &read_fds, &write_fds, &err_fds, &timeout);
      if (ret == 0)
        continue;

      if (FD_ISSET(client_fd, &err_fds))
      {
        LOG("socket error");
        close(client_fd);
        break;
      }

      if (capture_handle && FD_ISSET(client_fd, &write_fds))
      {
//        ssize_t n = write(client_fd, &capture_buffer[0], capture_buffer.size());
        ssize_t n = send(client_fd, &capture_buffer[0], capture_buffer.size(), MSG_NOSIGNAL);

        if (n != static_cast<ssize_t>(capture_buffer.size()))
        {
          if (n == -1)
          {
            LOG("error sending on socket: %s", strerror(errno));
          }
          else
          {
            LOG("failed to send all audio data. only sent %d of %d bytes.",
                static_cast<int>(n), static_cast<int>(capture_buffer.size()));
          }
          close(client_fd);
          break;
        }
      }
      else
      {
        LOG("client isn't keeping up");
      }

      if (FD_ISSET(client_fd, &read_fds))
      {
        int n = read(client_fd, &playback_buffer[0], playback_buffer_size);
        if (n > 0)
        {
          if (playback_buffer_read == playback_buffer_size)
          {
            snd_pcm_writei(playback_handle, &playback_buffer[0], n/2);
          }
        }
      }
    }

    close(client_fd);
  }

  snd_pcm_close(capture_handle);
  return 0;
}
