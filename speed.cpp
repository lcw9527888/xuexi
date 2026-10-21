#include <opencv2/opencv.hpp>   // OpenCV 总头文件：视频读取、绘制、跟踪器
#include <opencv2/tracking.hpp> //报错后引入追踪头文件
#include <iostream>             // 用于 std::cerr 输出错误
#include <algorithm>            // std::clamp 需要
#include <cstdio>               // std::snprintf 需要

int main() {
    // ===== 1. 参数（硬编码，省掉配置文件）=====
    const std::string videoPath = "videos/example.mp4";  // 视频路径，相对运行目录
    const double Kp   = 0.8;                          // 比例系数：偏差→速度的放缩倍数
    const double vMax = 1.0;                          // 虚拟速度限幅，防止指令爆掉

    // ===== 2. 打开视频，读第一帧 =====
    cv::VideoCapture cap(videoPath);                  // 构造即打开视频文件
    if (!cap.isOpened()) {                            // 检查是否打开成功
        std::cerr << "cannot open video\n";           // 路径错/格式不支持会走到这
        return -1;
    }

    cv::Mat frame;                                    // 存放当前帧图像
    if (!cap.read(frame)) {                           // 读第一帧，失败说明文件空或损坏
        std::cerr << "cannot read frame\n";
        return -1;
    }

    // ===== 3. 鼠标选目标，初始化跟踪器 =====
    // 弹窗显示第一帧，鼠标拖框选目标，回车确认，返回 (x, y, w, h)
    cv::Rect bbox = cv::selectROI("select target", frame, false);
    if (bbox.width <= 0) return 0;                    // 用户按 ESC 没选，直接退出

    // CSRT 是 OpenCV 自带跟踪器，不需要训练模型，给首帧目标框即可逐帧跟踪
    cv::Ptr<cv::Tracker> tracker = cv::TrackerCSRT::create();
    tracker->init(frame, bbox);                       // 告诉跟踪器首帧目标位置，提取特征模板

    // ===== 4. 逐帧循环 =====
    while (cap.read(frame)) {                         // 每读一帧执行一次，视频结束自动退出
        cv::Rect cur;                               // 跟踪器输出的当前帧目标框
        bool ok = tracker->update(frame, cur);        // true=跟踪成功，false=目标丢失

        // 先置零，保证目标丢失时速度为 0
        double ex = 0, ey = 0, vx = 0, vy = 0;

        // ----- 分支 A：跟踪成功 -----
        if (ok) {
            bbox = cur;                               // 用最新结果更新目标框

            // 目标框中心点坐标
            double cx = bbox.x + bbox.width  / 2.0;
            double cy = bbox.y + bbox.height / 2.0;

            // 归一化偏差：落在约 [-1,1]，不随分辨率变化
            // ex>0 目标在右，ex<0 在左；ey>0 目标在下
            ex = (cx - frame.cols / 2.0) / (frame.cols / 2.0);
            ey = (cy - frame.rows / 2.0) / (frame.rows / 2.0);

            // 比例控制 + 限幅：负号表示“目标偏右时向左修正”
            vx = std::clamp(-Kp * ex, -vMax, vMax);
            vy = std::clamp(-Kp * ey, -vMax, vMax);
        // ----- 分支 B：目标丢失 -----
        } else {
            // 速度保持 0，画面提示丢失
            cv::putText(frame, "LOST", {20, 40},
                        cv::FONT_HERSHEY_SIMPLEX, 1.0, {0, 0, 255}, 2);
        }

        // ===== 5. 可视化（仅在跟踪成功时绘制框和偏差线）=====
        if (ok) {
            cv::rectangle(frame, bbox, {0, 255, 0}, 2);           // 绿色目标框（BGR）
            cv::Point tc(bbox.x + bbox.width / 2,                 // 目标中心点
                         bbox.y + bbox.height / 2);
            cv::Point cc(frame.cols / 2, frame.rows / 2);         // 画面中心点
            cv::circle(frame, tc, 4, {0, 0, 255}, -1);            // 红色实心点标目标中心
            cv::line(frame, cc, tc, {255, 0, 0}, 1);              // 蓝线连接两中心，线越长偏差越大
        }

        // 左上角显示当前偏差和虚拟速度数值
        char text[128];
        std::snprintf(text, sizeof(text), "ex=%.2f ey=%.2f vx=%.2f vy=%.2f",
                      ex, ey, vx, vy);
        cv::putText(frame, text, {20, 80},
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, {0, 255, 255}, 1);

        // ===== 6. 显示与按键 =====
        cv::imshow("tracking", frame);                    // 显示处理后的帧
        int key = cv::waitKey(1) & 0xFF;                  // 等 1ms 并读键，同时刷新窗口
        if (key == 'q' || key == 27) break;               // q 或 ESC 退出
    }

    // ===== 7. 释放资源 =====
    cap.release();                                        // 关闭视频，释放解码器
    cv::destroyAllWindows();                              // 关闭所有 OpenCV 窗口
    return 0;
}
