#include "HostEvent.h"

#include <rsutils/json.h>
using nlohmann::json;


HostEvent* HostEvent::gInstance = nullptr;


HostEvent::HostEvent() :
        mHost(nullptr),
        mInitialied(false)
{
    mHost = new Host();
    memset(mCamSetting, 0, sizeof(mCamSetting));
    gInstance = this;
}

HostEvent::~HostEvent() {
    mInitialied = false;
    delete mHost;
}

HostEvent* HostEvent::getInstance() {
    if (gInstance) {
        return gInstance;
    }
    return nullptr;
}

bool HostEvent::init(CamSetting *setting) {
    json * jsonData = nullptr;
    if (setting->id == RGB_CAMERA) {
        jsonData = &mRgbCommand;
    } else if (setting->id == 1) {
        jsonData = &mDepthCommand;
    } else {
        printf("failed to init jsonData\n");
        return false;
    }
            
    json & Root = *jsonData;
    auto& si = Root["StreamInfo"];
    si["width"] = setting->width;
    si["height"] = setting->height;
    si["fps"] = setting->fps;
    si["command"] = "CLOSE";

    if (setting->id == RGB_CAMERA) {
        si["format"] = "YUYV";
        si["camera"] = "RGB";
    } else {
        si["format"] = "Z16";
        si["camera"] = "DEPTH";
    }
    mCamSetting[setting->id] = setting;

    return true;
}

bool HostEvent::initDDS() {
    HostConfig config;
    config.RGBReadyCB = [this]( HeaderBlob1 & st )
    {
        printf( "%s, receive RGB data: %p", __func__, st.data().data() );
        CamSetting * setting = ( getInstance()->mCamSetting[RGB_CAMERA] );
        printf( "%s, data info: %dx%d, fps%d, frameNum: %u",
                __func__,
                setting->width,
                setting->height,
                setting->fps,
                setting->frameNum );

        if( setting->frameNum % FPS_FRAME_COUNT == 0 )
        {
            auto duration = realdds::now() - setting->frameTime;
            float curFps
                = static_cast< float >( 1000000000 ) / static_cast< float >( duration.to_ns() / FPS_FRAME_COUNT );
            printf( "@%s, RGB fps: %02f\n", __func__, curFps );
            setting->frameTime = realdds::now();
        }

        setting->frameNum++;
        setting->bufLock.lock();
        convertYUY2Fmt( setting->width, setting->height, st.data().data(), setting->buf );
        setting->bufLock.unlock();

        std::unique_lock< std::mutex > lock( _frame_mutex );
        _have_new_frame = true;
        _frame_cv.notify_all();
    };
    config.DepthReadyCB = [this]( HeaderBlob1 & st )
    {
        printf( "%s, receive DEPTH data: %p", __func__, st.data().data() );
        CamSetting * setting = ( getInstance()->mCamSetting[DEPTH_CAMERA] );
        printf( "%s, data info: %dx%d, fps%d, frameNum: %u",
                __func__,
                setting->width,
                setting->height,
                setting->fps,
                setting->frameNum );

        if( setting->frameNum % FPS_FRAME_COUNT == 0 )
        {
            auto duration = realdds::now() - setting->frameTime;
            float curFps
                = static_cast< float >( 1000000000 ) / static_cast< float >( duration.to_ns() / FPS_FRAME_COUNT );
            printf( "@%s, DEPTH fps: %02f\n", __func__, curFps );
            setting->frameTime = realdds::now();
        }

        setting->frameNum++;
        setting->bufLock.lock();
        convertZ162RGB( setting->width, setting->height, st.data().data(), setting->buf );
        setting->bufLock.unlock();

        std::unique_lock< std::mutex > lock( _frame_mutex );
        _have_new_frame = true;
        _frame_cv.notify_all();
    };

    if (!mHost->init(config)) {
        printf("failed to init host\n");
        return false;
    }

    if (!mHost->waitDevice()) {
        printf("failed to waitDevice\n");
        return false;
    }

    // need to sent dummy command firstly
    mHost->sendCommand("dummy", __func__ );

    mInitialied = true;
    return true;
}

bool HostEvent::sendHostEvent(CamSetting *setting) {
    if (!mInitialied) {
        printf("%s, Host is not initialied\n", __func__);
        return false;
    }

    json *jsonData = nullptr;
    if (setting->id == RGB_CAMERA) {
        jsonData = &mRgbCommand;
    } else if (setting->id == DEPTH_CAMERA) {
        jsonData = &mDepthCommand;
    } else {
        printf("failed to init jsonData\n");
        return false;
    }
            
    json & Root = *jsonData;
    auto& si = Root["StreamInfo"];
    si["width"] = setting->width;
    si["height"] = setting->height;
    si["fps"] = setting->fps;
    if (setting->id == RGB_CAMERA) {
        si["format"] = "YUYV";
        si["camera"] = "RGB";
    } else {
        si["format"] = "Z16";
        si["camera"] = "DEPTH";
    }

    if (setting->start) {
        si["command"] = "OPEN";
        mHost->sendCommand(Root.dump(), __func__ );
        std::this_thread::sleep_for( std::chrono::microseconds( 500000 ) );

        si["command"] = "START";
        mHost->sendCommand(Root.dump(), __func__ );
        setting->frameNum = 0;
        setting->frameTime = realdds::now();
    } else {
        si["command"] = "STOP";
        mHost->sendCommand(Root.dump(), __func__ );
        std::this_thread::sleep_for( std::chrono::microseconds( 500000 ) );

        si["command"] = "CLOSE";
        mHost->sendCommand(Root.dump(), __func__ );
    }

    return true;
}

void HostEvent::YUV2RGB(unsigned char Y, unsigned char U, unsigned char V,
                        unsigned char* R, unsigned char* G, unsigned char* B) {
    int Yp, Up, Vp, Ypp;
    int oR, oG, oB;
    Yp = Y - 16;
    Up = (U - 128);
    Vp = (V - 128);
    Ypp = 9535 * Yp;

    oB = (Ypp + 16531 * Up) >> 13;
    oG = (Ypp - 6660 * Vp - 3203 * Up) >> 13;
    oR = (Ypp + 13074 * Vp) >> 13;
    if (oR > 255) oR = 255;
    if (oR < 0) oR = 0;
    if (oG > 255) oG = 255;
    if (oG < 0) oG = 0;
    if (oB > 255) oB = 255;
    if (oB < 0) oB = 0;
    *R = (unsigned char)oR; *G = (unsigned char)oG; *B = (unsigned char)oB;
}

void HostEvent::convertYUY2Fmt(int width, int height, unsigned char *inBuf, unsigned char* outBuf) {
    int srcStride = width * 2;
    int dstStride = width * 4;
    for (auto y = 0; y < height; y++) {
        for (auto x = 0; x < width; x++) {
            unsigned char Y[2];
            unsigned char U;
            unsigned char V;
            unsigned char R, G, B;
            Y[0] = inBuf[y * srcStride + x * 2];
            Y[1] = inBuf[y * srcStride + x * 2 + 2];
            U = inBuf[y * srcStride + x * 2 + 1];
            V = inBuf[y * srcStride + x * 2 + 3];

            YUV2RGB(Y[0], U, V, &R, &G, &B);
            outBuf[y * dstStride + x * 4] = B;
            outBuf[y * dstStride + x * 4 + 1] = G;
            outBuf[y * dstStride + x * 4 + 2] = R;

            x++;
            YUV2RGB(Y[1], U, V, &R, &G, &B);
            outBuf[y * dstStride + x * 4] = B;
            outBuf[y * dstStride + x * 4 + 1] = G;
            outBuf[y * dstStride + x * 4 + 2] = R;
        }
    }
}

void HostEvent::convertZ162RGB(int width, int height, unsigned char *inBuf, unsigned char *outBuf) {
    unsigned short* pData = (unsigned short*)inBuf;
    for (int i = 0; i < width * height; i++) {
        unsigned short d = pData[i] >> 5;
        unsigned char v;
        if (d > 255)
            v = 255;
        else
            v = (unsigned char)d;
        outBuf[i * 4] = outBuf[i * 4 + 1] = outBuf[i * 4 + 2] = outBuf[i * 4 + 3] = v;
    }
}

void HostEvent::waitForFrame()
{
    std::unique_lock< std::mutex > lock( _frame_mutex );
    _frame_cv.wait( lock, [this]() { return ! mInitialied || _have_new_frame; } );
    _have_new_frame = false;
}
