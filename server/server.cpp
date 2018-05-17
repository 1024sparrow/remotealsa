#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string>
#include <vector>

static int capture_buffer_frames = 128;
static snd_pcm_t* capture_handle = NULL;
static std::vector<uint8_t> capture_buffer;

#define D(FUNC) if ((err = FUNC) < 0) {\
    fprintf(stderr, "[%s:%d] -- %s (%d):%s\n", __FILE__, (__LINE__ - 1), #FUNC, err, snd_strerror(err)); \
    exit(1);\
  } else { \
    printf("%s : ok\n", #FUNC); \
  }


static void setup_capture(char const* capture_handle_name)
{
  int err;
  uint32_t sample_rate = 441000;
  snd_pcm_hw_params_t* params;
  snd_pcm_format_t fmt = SND_PCM_FORMAT_S16_LE;

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


int main(int argc, char* argv[])
{
  int err;
  int server_fd;
  int port = 10001;
  struct sockaddr_in server_addr;

  std::string capture_handle_name = "hw:1,0";

  setup_capture(capture_handle_name.c_str());

  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  err = bind(server_fd, reinterpret_cast<struct sockaddr *>(&server_addr), sizeof(server_addr));
  if (err < 0)
  {
    fprintf(stderr, "failed to bind listen socket. %s", strerror(err));
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
      fprintf(stderr, "error accepting client connection. %s", strerror(errno));
      continue;
    }

    while (true)
    {
      // std::fill(capture_buffer.begin(), capture_buffer.end(), 0);
      err = snd_pcm_readi(capture_handle, &capture_buffer[0], capture_buffer_frames);
      if (err != capture_buffer_frames)
      {
	fprintf(stderr, "error reading from capture handle. %s", snd_strerror(err));
	exit(1);
      }

      ssize_t n = write(client_fd, &capture_buffer[0], capture_buffer.size());
      if (n != static_cast<ssize_t>(capture_buffer.size()))
      {
	if (n == -1)
	{
	  fprintf(stderr, "error sending on socket. %s\n", strerror(err));
	}
	else
	{
	  fprintf(stderr, "failed to send all audio data. only sent %d of %d bytes.\n",
	      static_cast<int>(n), static_cast<int>(capture_buffer.size()));
	}
	break;
      }


      #if 0
      for (int i = 0; i < capture_buffer.size(); ++i)
      {
	printf("%02x ", capture_buffer[i]);
	if ((i+1) % 32 == 0)
	  printf("\n");
      }
      printf("\n\n");
      #endif
    }

    close(client_fd);
  }

  snd_pcm_close(capture_handle);
  return 0;
}
