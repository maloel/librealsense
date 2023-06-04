#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>

#include "Host.h"
#include "UddsPubSubTypes.h"

#include <rsutils/json.h>
using nlohmann::json;

using namespace eprosima::fastdds::dds;

Host::Host()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , devinfo_topic_(nullptr)
    , rgb_topic_(nullptr)
    , depth_topic_(nullptr)
    , cmd_topic_(nullptr)
    , devinfo_reader_(nullptr)
    , rgb_reader_(nullptr)
    , depth_reader_(nullptr)
    , cmd_writer_(nullptr)
    , blob_type_(new BlobPubSubType())
    , headerblob1_type_(new HeaderBlob1PubSubType())
{
    writer_listener_.owner = this;
    reader_listener_.owner = this;
    RGBReadyCB_ = nullptr;
    DepthReadyCB_ = nullptr;
}

Host::~Host()
{
    if (devinfo_reader_ != nullptr)
    {
        subscriber_->delete_datareader(devinfo_reader_);
    }
    if (rgb_reader_ != nullptr)
    {
        subscriber_->delete_datareader(rgb_reader_);
    }
    if (depth_reader_ != nullptr)
    {
        subscriber_->delete_datareader(depth_reader_);
    }
    if (cmd_writer_ != nullptr)
    {
        publisher_->delete_datawriter(cmd_writer_);
    }
    if (devinfo_topic_ != nullptr)
    {
        participant_->delete_topic(devinfo_topic_);
    }
    if (rgb_topic_ != nullptr)
    {
        participant_->delete_topic(rgb_topic_);
    }
    if (depth_topic_ != nullptr)
    {
        participant_->delete_topic(depth_topic_);
    }
    if (cmd_topic_ != nullptr)
    {
        participant_->delete_topic(cmd_topic_);
    }
    if (subscriber_ != nullptr)
    {
        participant_->delete_subscriber(subscriber_);
    }
    if (publisher_ != nullptr)
    {
        participant_->delete_publisher(publisher_);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

bool Host::init(HostConfig config)
{
    RGBReadyCB_ = config.RGBReadyCB;
    DepthReadyCB_ = config.DepthReadyCB;
    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;
    pqos.name("Host");

#if 0
    auto transportDescriptor = std::make_shared<eprosima::fastdds::rtps::UDPv4TransportDescriptor>();
    transportDescriptor->interfaceWhiteList.emplace_back("192.168.11.4");
    pqos.transport().user_transports.push_back(transportDescriptor);
    pqos.transport().use_builtin_transports = false;
#endif

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);
    if (participant_ == nullptr)
    {
        return false;
    }

    //REGISTER THE TYPE
    blob_type_.register_type(participant_);
    headerblob1_type_.register_type(participant_);

    devinfo_topic_ = participant_->create_topic(
        "rs/device_info",
        blob_type_.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (devinfo_topic_ == nullptr)
    {
        return false;
    }

    //CREATE THE SUBSCRIBER
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    if (subscriber_ == nullptr)
    {
        return false;
    }

    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    if (publisher_ == nullptr)
    {
        return false;
    }

    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    devinfo_reader_ = subscriber_->create_datareader(devinfo_topic_, rqos, &reader_listener_);
    if (devinfo_reader_ == nullptr)
    {
        return false;
    }

    return true;
}

bool Host::waitDevice()
{
    //CREATE THE TOPIC
    char name[256];

    std::cout << "Waiting for device" << std::endl;
    while (!device_found_)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    sprintf(name, "rs/%s/rgb", sn_.c_str());
    std::cout << "create topic " << name << std::endl;
    rgb_topic_ = participant_->create_topic(
        name,
        headerblob1_type_.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (rgb_topic_ == nullptr)
    {
        return false;
    }

    sprintf(name, "rs/%s/depth", sn_.c_str());
    std::cout << "create topic " << name << std::endl;
    depth_topic_ = participant_->create_topic(
        name,
        headerblob1_type_.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (depth_topic_ == nullptr)
    {
        return false;
    }

    sprintf(name, "rs/%s/cmd", sn_.c_str());
    std::cout << "create topic " << name << std::endl;
    cmd_topic_ = participant_->create_topic(
        name,
        blob_type_.get_type_name(),
        TOPIC_QOS_DEFAULT);
    if (cmd_topic_ == nullptr)
    {
        return false;
    }

    //CREATE THE READER
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rgb_reader_ = subscriber_->create_datareader(rgb_topic_, rqos, &reader_listener_);
    if (rgb_reader_ == nullptr)
    {
        std::cout << "Failed to create rgb reader" << std::endl;
        return false;
    }
    depth_reader_ = subscriber_->create_datareader(depth_topic_, rqos, &reader_listener_);
    if (depth_reader_ == nullptr)
    {
        std::cout << "Failed to create depth reader" << std::endl;
        return false;
    }

    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    cmd_writer_ = publisher_->create_datawriter(cmd_topic_, wqos, &writer_listener_);
    if (cmd_writer_ == nullptr)
    {
        std::cout << "Failed to create cmd writer" << std::endl;
        return false;
    }
    return true;
}

void Host::ReaderListener::on_subscription_matched(
        DataReader* reader,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched = info.total_count;
        std::cout << (reader == owner->rgb_reader_ ? "RGB" : "Depth") << " subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched = info.total_count;
        std::cout << (reader == owner->rgb_reader_ ? "RGB" : "Depth") << " subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void Host::ReaderListener::on_data_available(
        DataReader* reader)
{
    if (reader == owner->devinfo_reader_)
    {
        // device info
        Blob st;
        SampleInfo info;

        if (reader->take_next_sample(&st, &info) == ReturnCode_t::RETCODE_OK)
        {
            if (info.valid_data && !owner->device_found_)
            {
                char const * begin = (char const *) st.data().data();
                char const * end = begin + st.data().size();
                auto j = json::parse( begin, end );
                std::cout << "received device info: " << j << std::endl;
                if( !j.is_object() )
                {
                    std::cout << "Not an object!" << std::endl;
                    return;
                }
                auto diit = j.find( "DeviceInfo" );
                if( diit == j.end() )
                {
                    std::cout << "'DeviceInfo' not found!" << std::endl;
                    return;
                }
                auto const & di = diit.value();
                if( !di.is_object() )
                {
                    std::cout << "'DeviceInfo' is not an object!" << std::endl;
                    return;
                }
                auto snit = di.find( "SN" );
                if( snit == di.end() )
                {
                    std::cout << "'DeviceInfo/SN' not found!" << std::endl;
                    return;
                }
                auto const & sn = snit.value();
                if( !sn.is_string() )
                {
                    std::cout << "'DeviceInfo/SN' is not a string!" << std::endl;
                    return;
                }

                owner->sn_ = sn.get< std::string >();
                std::cout << "Found device. sn: " << owner->sn_ << std::endl;

                // create topics/reader/writers for this device
                owner->device_found_ = true;
            }
        }
    }
    else
    {
        // image frame
        HeaderBlob1 st;
        SampleInfo info;

        if (reader->take_next_sample(&st, &info) == ReturnCode_t::RETCODE_OK)
        {
            if (info.valid_data)
            {
                // Print your structure data here.
                ++samples;
                ImageHeader* pHeader = reinterpret_cast<ImageHeader*>(st.header().data());
                std::cout << (reader == owner->rgb_reader_ ? "RGB" : "Depth")
                    << " frame received. timestamp: " << pHeader->timestamp
                    << " index: " << pHeader->index
                    << " size: " << st.data().size() << std::endl;
                if (reader == owner->rgb_reader_ && owner->RGBReadyCB_) {
                    printf("%s, call RGB ready callback\n", __func__);
                    owner->RGBReadyCB_(st);
                }
                if (reader == owner->depth_reader_ && owner->DepthReadyCB_) {
                    printf("%s, call Depth ready callback\n", __func__);
                    owner->DepthReadyCB_(st);
                }
            }
        }
    }
}

void Host::WriterListener::on_publication_matched(eprosima::fastdds::dds::DataWriter* writer, const eprosima::fastdds::dds::PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched = info.total_count;
        std::cout << "Publication matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched = info.total_count;
        std::cout << "Publication unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
            << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

void gen_cmd(Blob& st, std::string const & cmd )
{
    auto& data = st.data();
    auto len = cmd.length();
    data.resize(len + 1);
    memcpy( data.data(), cmd.c_str(), len + 1 );
}

bool Host::sendCommand(std::string const & cmd, char const * func)
{
    std::cout << func << ", send command: " << cmd << std::endl;
    Blob st;
    gen_cmd(st, cmd);
    return cmd_writer_->write(&st);
}


