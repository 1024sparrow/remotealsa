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
#include <getopt.h>

#include <string>
#include <vector>

static int capture_buffer_frames = 128;
static snd_pcm_t* capture_handle = NULL;
static std::vector<uint8_t> capture_buffer;
static snd_pcm_t* playback_handle = NULL;
static std::vector<uint8_t> playback_buffer;
static int playback_buffer_size;
static int playback_buffer_read;
static snd_pcm_uframes_t playback_frames;

#define D(FUNC) if ((err = FUNC) < 0) {\
    printf("[%s:%d] -- %s (%d):%s\n", __FILE__, (__LINE__ - 1), #FUNC, err, snd_strerror(err)); \
    exit(1);\
  } else { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    printf("%ld.%04ld LOG:(%04d) -- ", tv.tv_sec, tv.tv_usec, __LINE__ - 4);\
    printf("%s : ok\n", #FUNC);\
  }

#define LOG(FORMAT, ...) \
  do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    printf("%ld.%04ld LOG:(%04d) -- ", tv.tv_sec, tv.tv_usec, __LINE__ - 2); \
    printf(FORMAT, ##__VA_ARGS__); \
    printf("\n"); \
  } while (0)

static void setup_capture(char const* capture_handle_name)
{
  int err;
  uint32_t sample_rate = 44100;
  snd_pcm_hw_params_t* params;
  snd_pcm_format_t fmt = SND_PCM_FORMAT_S16_LE;

  LOG("setup_capture with device:%s", capture_handle_name);

  D( snd_pcm_open(&capture_handle, capture_handle_name, SND_PCM_STREAM_CAPTURE, 0) );
  D( snd_pcm_hw_params_malloc(&params) );
  D( snd_pcm_hw_params_any(capture_handle, params) );
  D( snd_pcm_hw_params_set_access(capture_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED) );
  D( snd_pcm_hw_params_set_format(capture_handle, params, fmt) );
  D( snd_pcm_hw_params_set_rate_near(capture_handle, params, &sample_rate, 0));
  D( snd_pcm_hw_params_set_channels(capture_handle, params, 2));
  D( snd_pcm_hw_params(capture_handle, params) );
  
  snd_pcm_hw_params_free(params);

  D( snd_pcm_prepare(capture_handle) );

  const uint32_t n = (capture_buffer_frames * (snd_pcm_format_width(fmt) / 8) * 2);
  capture_buffer.reserve(n);
  capture_buffer.resize(n);
}

static void setup_playback(char const* playback_handle_name)
{
  int err;
  int num_channels = 2;
  uint32_t period_time;
  uint32_t sample_rate = 44100;
  snd_pcm_hw_params_t* params;
  snd_pcm_uframes_t playback_frames;

  LOG("setup_playback with device:%s", playback_handle_name);

  D( snd_pcm_open(&playback_handle, playback_handle_name, SND_PCM_STREAM_PLAYBACK, 0) );
  snd_pcm_hw_params_alloca(&params);
  D( snd_pcm_hw_params_set_format(playback_handle, params, SND_PCM_FORMAT_S16_LE) );
  D( snd_pcm_hw_params_set_channels(playback_handle, params, num_channels) );
  D( snd_pcm_hw_params_set_rate_near(playback_handle, params, &sample_rate, 0) );
  D( snd_pcm_hw_params(playback_handle, params) );
  D( snd_pcm_hw_params_get_period_size(params, &playback_frames, 0) );

  const uint32_t n = (playback_frames * num_channels * 2);
  playback_buffer.reserve(n);
  playback_buffer.resize(n);
  playback_buffer_size = n;
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
    { "playback", required_argument, NULL, 'p' },
    { 0, 0, 0, 0 }
  };

  int c;
  while ((c = getopt_long(argc, argv, "c:p:", long_options, NULL)) != -1)
  {
    switch (c)
    {
      case 'c':
        capture_device = optarg;
        break;
      case 'p':
        playback_device = optarg;
        break;
      case 10000:
        port = static_cast<int>(strtol(optarg, NULL, 10));
        break;
      case '?':
        printf("invalid option\n");
        break;
    }
  }

  setup_capture(capture_device);
  setup_playback(playback_device);

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  err = bind(server_fd, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr));
  if (err < 0)
  {
    LOG("failed to bind listen socket. %s", strerror(errno));
    exit(1);
  }

  listen(server_fd, 2);

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

    LOG("accepted connection");

    while (true)
    {
      // std::fill(capture_buffer.begin(), capture_buffer.end(), 0);
      err = snd_pcm_readi(capture_handle, &capture_buffer[0], capture_buffer_frames);
      if (err != capture_buffer_frames)
      {
        LOG("error reading from capture handle: %s", snd_strerror(err));
        exit(1);
      }

      fd_set read_fds;
      fd_set write_fds;
      fd_set err_fds;
      FD_ZERO(&write_fds);
      FD_ZERO(&read_fds);
      FD_ZERO(&err_fds);
      FD_SET(client_fd, &read_fds);
      FD_SET(client_fd, &write_fds);
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

      if (FD_ISSET(client_fd, &write_fds))
      {
        ssize_t n = write(client_fd, &capture_buffer[0], capture_buffer.size());
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

      if (FD_ISSET(client_fd, &read_fds))
      {
        int bytes_to_read = (playback_buffer_size - playback_buffer_read);
        int n = read(client_fd, &playback_buffer[0] + playback_buffer_read, bytes_to_read);
        if (n > 0)
        {
          playback_buffer_read += n;
          if (playback_buffer_read == playback_buffer_size)
          {
            snd_pcm_writei(playback_handle, &playback_buffer[0], playback_frames);
          }
        }
      }
    }

    close(client_fd);
  }

  snd_pcm_close(capture_handle);
  return 0;
}
