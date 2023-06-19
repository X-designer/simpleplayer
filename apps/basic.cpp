extern "C" {
#include <SDL.h>
#include <SDL_keyboard.h>
#include <SDL_render.h>
#include <SDL_video.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <iostream>

using namespace std;

static const std::string kEngineName = "xTutor";

class SimplePlayer final {
 public:
  SimplePlayer(std::string& title, int x, int y, int w, int h, uint32_t flags) {
    win_rect_ = {x, y, w, h};

    window_ = SDL_CreateWindow(title.c_str(), x, y, w, h, flags);
    renderer_ = SDL_CreateRenderer(window_, -1, 0);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 255, 0);
  }

  void Init() {
    fmt_context_ = avformat_alloc_context();
    int result = avformat_open_input(&fmt_context_, media_name_.c_str(),
                                     nullptr, nullptr);

    AVStream* v_stream = fmt_context_->streams[0];

    const AVCodec* codec = nullptr;
    AVCodecParserContext* parser;
    AVCodecParameters* vidpart = nullptr;

    if (result < 0) {
      cout << "avformat open input error" << endl;
      return;
    }
    cout << "stream count: " << fmt_context_->nb_streams << endl;

    codec = avcodec_find_decoder(v_stream->codecpar->codec_id);

    parser = av_parser_init(codec->id);

    pkt_ = av_packet_alloc();

    vidpart = v_stream->codecpar;
    v_ctx_ = avcodec_alloc_context3(codec);
    result = avcodec_parameters_to_context(v_ctx_, vidpart);
    if (result < 0) {
      cout << "avformat parameters to context error" << endl;
      return;
    }
    avcodec_open2(v_ctx_, codec, nullptr);
    frame_ = av_frame_alloc();

    while (av_read_frame(fmt_context_, pkt_) >= 0) {
      if (pkt_->stream_index != 0) {
        av_packet_unref(pkt_);
        continue;
      }
      avcodec_send_packet(v_ctx_, pkt_);
      avcodec_receive_frame(v_ctx_, frame_);

      // av_dump_format(fmt_context_, 0, media_name_.c_str(), 0);

      if (texture_ != nullptr) {
        break;
      }

      texture_ = SDL_CreateTexture(
          renderer_, SDL_PIXELFORMAT_IYUV,
          SDL_TEXTUREACCESS_STREAMING | SDL_TEXTUREACCESS_TARGET, 1920, 1080);

      SDL_UpdateYUVTexture(texture_, nullptr, frame_->data[0],
                           frame_->linesize[0], frame_->data[1],
                           frame_->linesize[1], frame_->data[2],
                           frame_->linesize[2]);

      cout << "codec name: " << codec->long_name << endl;
      cout << "video parameters size:(" << vidpart->width << ", "
           << vidpart->height << "). ";
      cout << "frame_ size:(" << frame_->width << ", " << frame_->height << ")"
           << endl;

      cout << "pkt_: " << pkt_->size << endl;
      av_packet_unref(pkt_);
    }
    cout << "init quit" << endl;
    is_running_ = false;
  }

  ~SimplePlayer() {
    if (fmt_context_ != nullptr) {
      avformat_close_input(&fmt_context_);
      avformat_free_context(fmt_context_);
    }

    if (texture_ != nullptr) SDL_DestroyTexture(texture_);
    if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
    if (window_ != nullptr) SDL_DestroyWindow(window_);
  }

  void SetMedia(std::string media_name) {
    media_name_ = std::move(media_name);
    cout << "media name : " << media_name_ << endl;
  }

  void Run() {
    Init();
    while (is_running_) {
      ProcessInput();
      Update();
    }
  }

  void ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          is_running_ = false;
          break;
        default:
          break;
      }
    }
  }

  void Update() {
    while (av_read_frame(fmt_context_, pkt_) >= 0) {
      if (pkt_->stream_index != 0) {
        av_packet_unref(pkt_);
        continue;
      }
      avcodec_send_packet(v_ctx_, pkt_);
      avcodec_receive_frame(v_ctx_, frame_);

      // av_dump_format(fmt_context_, 0, media_name_.c_str(), 0);

      SDL_UpdateYUVTexture(texture_, nullptr, frame_->data[0],
                           frame_->linesize[0], frame_->data[1],
                           frame_->linesize[1], frame_->data[2],
                           frame_->linesize[2]);

      cout << "frame_ size:(" << frame_->width << ", " << frame_->height << ")"
           << endl;

      cout << "pkt_: " << pkt_->size << endl;
      av_packet_unref(pkt_);
      break;
    }
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);

    SDL_Delay(33);
  }

 public:
  bool is_running_ = true;
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;

  SDL_Texture* texture_ = nullptr;

  SDL_Rect win_rect_;

  AVFormatContext* fmt_context_ = nullptr;
  AVPacket* pkt_ = nullptr;
  AVCodecContext* v_ctx_ = nullptr;
  AVFrame* frame_ = nullptr;

  std::string media_name_;
};

void Demo01() {
  cout << "indent: " << LIBAVCODEC_IDENT << endl;
  cout << "build: " << LIBAVCODEC_VERSION_INT << endl;
}

void ShowHelp() {
  cout << R"(Usage:
	basic filename ...)"
       << endl;
}

std::string filename;

void Demo02() {}

std::string title = "simple player";

int main(int argc, char* argv[]) {
  cout << "ffmpeg sdl demo" << endl;

  if (argc < 2) {
    ShowHelp();
    return 1;
  }
  filename = std::move(string(argv[1]));

  cout << "media file name: " << filename << endl;

  SimplePlayer player(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                      800, 600, 0);
  player.SetMedia(argv[1]);
  player.Run();
  return 0;
}
