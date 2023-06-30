#include <chrono>
#include <iostream>
#include <thread>

extern "C" {
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_ttf.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace std;

const std::string app_title("hello app");
const int WIDTH = 800;
const int HEIGHT = 600;

bool is_quit = false;

std::string media_name;

void Delay(uint32_t count) {
  this_thread::sleep_for(std::chrono::milliseconds(count));
}

class SimplePlayer final {
 public:
  void ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          is_quit_ = true;
          break;
        case SDL_KEYDOWN:
          if (event.key.keysym.sym == SDLK_f) {
            static bool is_full = false;
            if (is_full) {
              SDL_SetWindowFullscreen(window_, 0);
            } else {
              SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
            }
            is_full = !is_full;
            cout << "toggle window" << endl;
          }
          break;
        default:
          break;
      }
    }
  }

  void Update() {
    int result = av_read_frame(av_format_context_, packet_);
    if (result < 0) {
      cout << "av read frame error: " << endl;
      is_quit_ = true;
      return;
    }

    float delytime =
        (fpsrendering != 0 && !isnan(fpsrendering)) ? fpsrendering : 33;

    // display video
    if (packet_->stream_index != 0) {
      av_packet_unref(packet_);
      return;
    }
    result = avcodec_send_packet(vid_context_, packet_);
    if (result < 0) {
      cout << "avcodec send packet error: " << endl;
      return;
    }

    result = avcodec_receive_frame(vid_context_, v_frame_);
    if (result < 0) {
      cout << "avcodec receive packet error: " << endl;
      return;
    }

    // int frame_num = vid_context_->frame_number;
    // cout << "Frame " << frame_num                 // frame_num
    //      << ", pkt_size: " << v_frame_->pkt_size  // pkt_size
    //      << ", pts" << v_frame_->pts              // pts
    //      << ", pkt_dts: " << v_frame_->pkt_dts    // pkt_dts
    //      << ", pts" << v_frame_->pts              //
    //      << ", pts" << v_frame_->pts << ", delay time" << delytime << endl;

    SDL_Rect rect{0, 0, vid_context_->width, vid_context_->height};
    cout << "rect: " << rect.w << ", " << rect.h << endl;
    SDL_RenderClear(renderer_);

    std::string msg = "hello tick: ";
    msg += std::to_string(SDL_GetTicks());
    SDL_Surface* surface = TTF_RenderText(font_, msg.c_str(), fg_, bg_);
    SDL_Texture* texttexture = SDL_CreateTextureFromSurface(renderer_, surface);

    SDL_UpdateYUVTexture(texture_, nullptr, v_frame_->data[0],
                         v_frame_->linesize[0], v_frame_->data[1],
                         v_frame_->linesize[1], v_frame_->data[2],
                         v_frame_->linesize[2]);

    SDL_Rect target{};
    SDL_GetWindowSize(window_, &target.w, &target.h);
    cout << "target: " << target.x << ", " << target.y << ", " << target.w
         << ", " << target.h << endl;
    // target.x = 50;
    // target.y = 50;
    // target.w -= target.x;
    // target.h -= target.y;
    SDL_RenderCopy(renderer_, texture_, nullptr, &target);

    SDL_Rect msg_rect{0, 0, 200, 100};
    // SDL_SetTextureBlendMode(texttexture, SDL_BLENDMODE_BLEND);
    // SDL_SetTextureAlphaMod(texttexture, 0x7F);
    SDL_RenderCopy(renderer_, texttexture, nullptr, &msg_rect);

    SDL_RenderPresent(renderer_);

    std::chrono::steady_clock::time_point t_point =
        std::chrono::steady_clock::now();

    // t_point = std::chrono::steady_clock::now();
    // auto during =
    //     chrono::duration_cast<chrono::milliseconds>(now -
    //     lastframe_timepoint_)
    //         .count();

    static uint32_t last_time = 0;
    if (last_time == 0) {
      last_time = SDL_GetTicks();
    } else {
      uint32_t now = SDL_GetTicks();
      uint32_t during = now - last_time;
      cout << "during: " << during << ", need sleep " << (delytime - during)
           << endl;

      this_thread::sleep_for(chrono::milliseconds(int(delytime - during)));
      // SDL_Delay(delytime - during);
      last_time = SDL_GetTicks();
    }

    //  lastframe_timepoint_ = std::chrono::steady_clock::now();

    av_packet_unref(packet_);
  }

  void Run() {
    if (media_name_ == "") {
      return;
    }

    av_format_context_ = avformat_alloc_context();
    int result = avformat_open_input(&av_format_context_, media_name_.c_str(),
                                     nullptr, nullptr);
    if (result < 0) {
      cout << "media result error: " << result << endl;
      return;
    }

    avformat_find_stream_info(av_format_context_, nullptr);

    for (int i = 0; i < av_format_context_->nb_streams; ++i) {
      AVCodecParameters* param = av_format_context_->streams[i]->codecpar;
      const AVCodec* codec = avcodec_find_decoder(param->codec_id);
      if (codec != nullptr)
        cout << "index : " << i << ", codec : " << codec->long_name << endl;
      else
        cout << "index : " << i << ", codec : " << param->codec_type << endl;
    }

    video_parm_ = av_format_context_->streams[0]->codecpar;
    video_codec_ = avcodec_find_decoder(video_parm_->codec_id);

    vid_context_ = avcodec_alloc_context3(video_codec_);
    result = avcodec_parameters_to_context(vid_context_, video_parm_);
    if (result < 0) {
      cout << "codec to context result error: " << result << endl;
      return;
    }

    result = avcodec_open2(vid_context_, video_codec_, nullptr);
    if (result < 0) {
      cout << "avcodec open2 error: " << result << endl;
      return;
    }

    // AVRational rational = av_format_context_->streams[0]->avg_frame_rate;
    AVRational rational = av_guess_frame_rate(
        av_format_context_, av_format_context_->streams[0], nullptr);

    fpsrendering = 1000.0 / (double(rational.num) / double(rational.den));

    v_frame_ = av_frame_alloc();
    packet_ = av_packet_alloc();

    v_rect_.w = video_parm_->width;
    v_rect_.h = video_parm_->height;

    texture_ = SDL_CreateTexture(
        renderer_, SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING | SDL_TEXTUREACCESS_TARGET,
        vid_context_->width, vid_context_->height);

    {
      cout << "stream 0" << endl;
      cout << "codec id : " << video_parm_->codec_id << endl;
      cout << "codec : " << video_codec_->long_name << endl;
      cout << "rational num : " << rational.num << endl;
      cout << "rational den : " << rational.den << endl;
      cout << "fpsrendering : " << fpsrendering << endl;
      cout << "video size: (" << vid_context_->width << ", "
           << vid_context_->height << ")" << endl;
    }

    lastframe_timepoint_ = std::chrono::steady_clock::now();
    while (!is_quit_) {
      ProcessInput();
      Update();
    }
  }

  void SetMediaName(std::string media_name) {
    cout << "media_name: " << media_name << endl;
    media_name_ = std::move(media_name);
  }

 public:
  SimplePlayer(std::string title, int x, int y, int w, int h, uint32_t flag)
      : title_(title) {
    cout << "sdl version: " << SDL_GetRevision() << endl;
    window_ = SDL_CreateWindow(title_.c_str(), x, y, w, h, flag);

    renderer_ = SDL_CreateRenderer(window_, -1, 0);

    TTF_Init();

    font_ = TTF_OpenFont("font/consola.ttf", 48);

    { SDL_SetRenderDrawColor(renderer_, 0, 255, 0, 100); }
  }

  ~SimplePlayer() {
    if (texture_ != nullptr) {
      SDL_DestroyTexture(texture_);
    }
    if (packet_ != nullptr) {
      av_packet_free(&packet_);
    }
    if (v_frame_ != nullptr) {
      av_frame_free(&v_frame_);
    }
    if (vid_context_ != nullptr) {
      avcodec_free_context(&vid_context_);
    }
    if (av_format_context_ != nullptr) {
      avformat_close_input(&av_format_context_);
      avformat_free_context(av_format_context_);
    }

    if (font_ != nullptr) TTF_CloseFont(font_);

    if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
    if (window_ != nullptr) SDL_DestroyWindow(window_);
  }

 public:
  std::string title_;

  std::string media_name_;

  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* texture_ = nullptr;

  TTF_Font* font_ = nullptr;

  SDL_Color fg_{0xff, 0xff, 0xff, 255};
  SDL_Color bg_{0x8e, 0x00, 0x00, 0x01};

  AVFormatContext* av_format_context_ = nullptr;

  const AVCodec* video_codec_ = nullptr;
  AVCodecParameters* video_parm_ = nullptr;
  AVCodecContext* vid_context_ = nullptr;
  AVFrame* v_frame_ = nullptr;
  AVPacket* packet_ = nullptr;

  std::chrono::steady_clock::time_point lastframe_timepoint_;
  double fpsrendering = 0;

  SDL_Rect v_rect_ = {0, 0, 0, 0};
  SDL_Rect win_rect_ = {0, 0, 0, 0};

  bool is_quit_ = false;
  bool show_playdetail_ = false;
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "usage: " << argv[0] << " <file name>\n" << std::endl;
    return EXIT_FAILURE;
  }

  SimplePlayer player(app_title.c_str(), SDL_WINDOWPOS_CENTERED,
                      SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
                      SDL_WINDOW_RESIZABLE);

  player.SetMediaName(argv[1]);
  if (argc > 2) {
    player.is_quit_ = true;
  }
  player.Run();
  return 0;
}
