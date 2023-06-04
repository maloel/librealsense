#pragma once

#include "Device.h"
#include "Host.h"
#include <nlohmann/json.hpp>
#include <mutex>
#include <realdds/dds-time.h>

enum {
    RGB_CAMERA,
    DEPTH_CAMERA,

    MAX_CAMERA_NUM
};

#define FPS_FRAME_COUNT 30

struct CamSetting
{
    int const id;
    char const * const name;
    bool start = false;
    int width;
    int height;
    int fps;

    unsigned char * const buf;
    std::mutex bufLock;
    uint32_t frameNum = 0;
    realdds::dds_time frameTime;

    CamSetting( int id_, char const * name_, int w, int h, int fps_ )
        : id( id_ )
        , name( name_ )
        , width( w )
        , height( h )
        , fps( fps_ )
        , buf( (unsigned char *)malloc( width * height * 4 ) )
    {
    }
};

class HostEvent {
    static void convertZ162RGB(int width, int height, unsigned char *inBuf, unsigned char *outBuf);
    static void convertYUY2Fmt(int width, int height, unsigned char *inBuf, unsigned char* outBuf);
    static void YUV2RGB(unsigned char Y, unsigned char U, unsigned char V,
                        unsigned char* R, unsigned char* G, unsigned char* B);
public:
    static HostEvent* getInstance();

public:
    HostEvent();
    virtual ~HostEvent();
    bool init(CamSetting *setting);
    bool sendHostEvent(CamSetting *setting);
    bool initDDS();
    void waitForFrame();

private:
    static HostEvent* gInstance;

    CamSetting * mCamSetting[MAX_CAMERA_NUM];
    nlohmann::json mRgbCommand;
    nlohmann::json mDepthCommand;
    Host *mHost;
    bool mInitialied;

    std::mutex _frame_mutex;
    std::condition_variable _frame_cv;
    bool _have_new_frame = false;
};
