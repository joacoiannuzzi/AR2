
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include <opencv2/imgproc.hpp>
#include <openvslam/system.h>

#include <openvslam/config.h>
#include <numeric>

#include "drawer3.h"
//#include "render_video_opengl2.h"

void camera_tracking(const std::shared_ptr<openvslam::config> &cfg,
                     const std::string &vocab_file_path, const unsigned int cam_num, const std::string &mask_img_path,
                     const float scale, const std::string &map_db_path) {
    // load the mask image
    const cv::Mat mask = mask_img_path.empty() ? cv::Mat{} : cv::imread(mask_img_path, cv::IMREAD_GRAYSCALE);

    // build a SLAM system
    openvslam::system SLAM(cfg, vocab_file_path);
    // startup the SLAM process
    SLAM.startup();

    std::cout << "LOG :: SLAM INITIALIZED" << std::endl;



    auto video = cv::VideoCapture(cam_num);
//    auto video = cv::VideoCapture(
//            "./resources/video.mp4",
//            cv::CAP_FFMPEG
//    );

    window_width = video.get(cv::CAP_PROP_FRAME_WIDTH);
    window_height = video.get(cv::CAP_PROP_FRAME_HEIGHT);
    std::cout << "Video width: " << window_width << std::endl;
    std::cout << "Video height: " << window_height << std::endl;

//    Drawer3 drawer(window_width, window_height);
    setup();

    std::cout << "LOG :: DRAWER INITIALIZED" << std::endl;


    if (!video.isOpened()) {
//        spdlog::critical("cannot open a camera {}", cam_num);
        SLAM.shutdown();
        std::cout << "VIDEO NOT OPENED" << std::endl;
        return;
    }

    cv::Mat frame;
    double timestamp = 0.0;
    std::vector<double> track_times;

    unsigned int num_frame = 0;

    bool is_not_end = true;
    // run the SLAM in another thread
//    std::thread thread([&]() {
    while (is_not_end) {
        // check if the termination of SLAM system is requested or not
        if (SLAM.terminate_is_requested()) {
            break;
        }

        is_not_end = video.read(frame);
        is_not_end = is_not_end && !shouldWindowClose();

        if (frame.empty()) {
            continue;
        }
        if (scale != 1.0) {
            cv::resize(frame, frame, cv::Size(), scale, scale, cv::INTER_LINEAR);
        }

        const auto tp_1 = std::chrono::steady_clock::now();

        // input the current currentFrame and estimate the camera pose
        auto pose = SLAM.feed_monocular_frame(frame, timestamp, mask);

        std::cout << "pose: " << pose << std::endl;

        update(frame, pose);

        const auto tp_2 = std::chrono::steady_clock::now();

        const auto track_time = std::chrono::duration_cast<std::chrono::duration<double>>(tp_2 - tp_1).count();
        track_times.push_back(track_time);

        timestamp += 1.0 / cfg->camera_->fps_;
        ++num_frame;
    }

    // wait until the loop BA is finished
    while (SLAM.loop_BA_is_running()) {
        std::this_thread::sleep_for(std::chrono::microseconds(5000));
    }
//    });

    // run the viewer in the current thread

//    viewer.run();


//    thread.join();

    // shutdown the SLAM process
    SLAM.shutdown();

    terminate();

    if (!map_db_path.empty()) {
        // output the map database
        SLAM.save_map_database(map_db_path);
    }

    std::sort(track_times.begin(), track_times.end());
    const auto total_track_time = std::accumulate(track_times.begin(), track_times.end(), 0.0);
    std::cout << "median tracking time: " << track_times.at(track_times.size() / 2) << "[s]" << std::endl;
    std::cout << "mean tracking time: " << total_track_time / track_times.size() << "[s]" << std::endl;
}


int main() {

    std::cout << "hello world" << std::endl;

    std::string folder = "./resources/";

    auto config_file = folder + "config.yaml";
    auto cfg = std::make_shared<openvslam::config>(config_file);

    auto vocab_file_path = folder + "orb_vocab.dbow2";

//    auto video_file_path = "gst-launch-1.0 udpsrc port=5000 caps=\"application/x-rtp\" ! rtph264depay ! avdec_h264 ! appsink";
//    auto video_file_path = "gst-launch-1.0 v4l2src device= /dev/video0 ! xvimagesink";
//    auto video_file_path = folder + "New House Video Tour 2014 - Under Construction.mp4";

    auto mask_img_path = "";
    auto map_db_path = folder + "map.msg";

    camera_tracking(cfg, vocab_file_path, 0, mask_img_path, 1, map_db_path);

//    Drawer drawer;
//    while (!drawer.shouldWindowClose()) {
//        drawer.update();
//    }

    return 0;
}




