#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>

#ifdef _WIN32
    #ifdef _WIN64
        #pragma comment(lib, "Processing.NDI.Lib.x64.lib")
    #else  // _WIN64
        #pragma comment(lib, "Processing.NDI.Lib.x86.lib")
    #endif  // _WIN64

    #include <windows.h>
#endif

#include "lib/NDI SDK for Linux/include/Processing.NDI.Lib.h"
#include "lib/screen_capture_lite/include/ScreenCapture.h"

std::map<int, void *> frame_buffers;
std::map<int, NDIlib_video_frame_v2_t> frames;
std::map<int, NDIlib_send_instance_t> senders;
std::map<int, std::thread> threads;

void ExtractAndConvertToRGBA(const SL::Screen_Capture::Image &img, unsigned char *dst, size_t dst_size) {
    size_t frame_size = static_cast<size_t>(SL::Screen_Capture::Width(img) * SL::Screen_Capture::Height(img) * sizeof(SL::Screen_Capture::ImageBGRA));
    assert(dst_size >= frame_size);
    memcpy(dst, SL::Screen_Capture::StartSrc(img), frame_size);
    return;
}

void ndi_sender(int monitor_id) {
    while (true) {
        NDIlib_send_send_video_v2(senders[monitor_id], &frames[monitor_id]);
    }
}

using namespace std::chrono_literals;
std::shared_ptr<SL::Screen_Capture::IScreenCaptureManager> dispCapture;

// static std::atomic<bool> exit_loop(false);
// static void sigint_handler(int) {
//     exit_loop = true;
// }

int main(int argc, char *argv[]) {
    if (!NDIlib_initialize()) {  // Cannot run NDI. Most likely because the CPU is not sufficient (see SDK documentation).
        printf("Cannot run NDI.");
        return 0;
    }

    // signal(SIGINT, sigint_handler);

    std::srand(std::time(nullptr));
    std::cout << "Running display capture" << std::endl;

    dispCapture = nullptr;
    dispCapture =
        SL::Screen_Capture::CreateCaptureConfiguration([]() {
            auto mons = SL::Screen_Capture::GetMonitors();
            std::cout << "Library is requesting the list of monitors to capture!" << std::endl;
            for (auto &m : mons) {
                NDIlib_send_create_t NDI_send_create_desc;

                const char* name = std::getenv("NDI_SOURCE_NAME");
                NDI_send_create_desc.p_ndi_name = name ? name : m.Name;

                NDIlib_send_instance_t pNDI_send = NDIlib_send_create(&NDI_send_create_desc);
                if (!pNDI_send) {
                    std::cout << "Error?" << std::endl;
                    continue;
                }

                // We are going to create a 1920x1080 interlaced frame at 29.97Hz.
                NDIlib_video_frame_v2_t NDI_video_frame;
                NDI_video_frame.xres = m.Width;
                NDI_video_frame.yres = m.Height;
                NDI_video_frame.FourCC = NDIlib_FourCC_type_BGRX;
                NDI_video_frame.line_stride_in_bytes = m.Width * 4;
                NDI_video_frame.frame_rate_N = 60000;
                NDI_video_frame.frame_rate_D = 1000;

                senders[m.Id] = pNDI_send;
                frames[m.Id] = NDI_video_frame;
                frame_buffers[m.Id] = malloc(m.Width * m.Height * 4);

                std::cout << m.Name << std::endl;

                threads[m.Id] = std::thread(ndi_sender, m.Id);
                threads[m.Id].detach();

                // free(p_frame_buffers[]);

                // NDIlib_send_destroy(pNDI_send);
                // NDIlib_destroy();

                // std::cout << m.Name << std::endl;
            }
            return mons;
        })
            ->onNewFrame([&](const SL::Screen_Capture::Image &img, const SL::Screen_Capture::Monitor &monitor) {
                if (!NDIlib_send_get_no_connections(senders[monitor.Id], 0)) {
                    return;
                }

                ExtractAndConvertToRGBA(img, (unsigned char *)frame_buffers[monitor.Id], monitor.Width * monitor.Height * 4);
                frames[monitor.Id].p_data = (uint8_t *)frame_buffers[monitor.Id];
                // NDIlib_send_send_video_v2(senders[monitor.Id], &frames[monitor.Id]);
                // NDIlib_send_send_video_async_v2(senders[monitor.Id], &frames[monitor.Id]);
            })
            ->start_capturing();

    dispCapture->setFrameChangeInterval(std::chrono::milliseconds(1));

    std::this_thread::sleep_until(std::chrono::system_clock::now() + std::chrono::hours(std::numeric_limits<int>::max()));

    return 0;
}
