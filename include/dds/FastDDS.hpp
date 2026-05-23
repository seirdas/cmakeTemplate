#pragma once

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/core/status/StatusMask.hpp>

#include "fastdds_generated/_ALL.hpp"

// Preparado para implementar la herencia en el stub
#ifndef FASTDDS
    namespace eprosima { namespace fastdds { namespace dds { class DataReader; } } }
#endif

template<typename MsgT>
class ReaderListenerT : public eprosima::fastdds::dds::DataReaderListener
{
public:
    using OnMsgFn = std::function<void(const MsgT&)>;
 
    explicit ReaderListenerT(OnMsgFn cb)
        : cb_(std::move(cb))
    {
    }
 
    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override
    {
        MsgT msg;
        eprosima::fastdds::dds::SampleInfo info;
 
        while (reader->take_next_sample(&msg, &info) ==
            eprosima::fastdds::dds::RETCODE_OK)
        {
            if (info.valid_data)
            {
                cb_(msg);
            }
        }
    }
 
private:
    OnMsgFn cb_;
};

class FastDDS {

// General ------------------------------------------------------------------------------

    /**
     * @brief Constructor estándar
     */
    FastDDS();

    /**
     * @brief Destructor estándar
     */
    ~FastDDS();

   
    void Inicializa_DDS();
    void writeCBSystems1();
 
private:
    void onCBSystems1InReceived(const CABINA_IDL::st_CircuitBreakersSystems1& msg);
    void onBreakerChangeInReceived(const CABINA_IDL::st_BreakerChangeEvent& msg);
    void onAvionicsSwInReceived(const CABINA_IDL::st_AvionicsSwitchesIn& msg);
    void onSystemTestResetInReceived(const CABINA_IDL::st_SystemTestResetIn& msg);
 
private:
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    eprosima::fastdds::dds::Publisher* publisher_ = nullptr;
 
    eprosima::fastdds::dds::Topic* topicCBSystems1In_ = nullptr;
    eprosima::fastdds::dds::Topic* topicCBSystems1Out_ = nullptr;
    eprosima::fastdds::dds::Topic* topicBreakerChangeIn_ = nullptr;
    eprosima::fastdds::dds::Topic* topicAvionicsSwIn_ = nullptr;
    eprosima::fastdds::dds::Topic* topicSystemTestResetIn_ = nullptr;
 
    eprosima::fastdds::dds::DataReader* CBSystems1Reader = nullptr;
    eprosima::fastdds::dds::DataReader* BreakerChangeReader = nullptr;
    eprosima::fastdds::dds::DataReader* AvionicsSwReader = nullptr;
    eprosima::fastdds::dds::DataReader* SystemTestResetReader = nullptr;

    eprosima::fastdds::dds::DataWriter* CBSystems1Writer = nullptr;
 
    eprosima::fastdds::dds::TypeSupport CBSystems1Type;
    eprosima::fastdds::dds::TypeSupport BreakerChangeType;
    eprosima::fastdds::dds::TypeSupport AvionicsSwType;
    eprosima::fastdds::dds::TypeSupport SystemTestResetType;
    eprosima::fastdds::dds::TypeSupport CBSystems1Type_Out;
 
    using CBSystems1Listener = ReaderListenerT<CABINA_IDL::st_CircuitBreakersSystems1>;
    using BreakerChangeListener = ReaderListenerT<CABINA_IDL::st_BreakerChangeEvent>;
    using AvionicsSwListener = ReaderListenerT<CABINA_IDL::st_AvionicsSwitchesIn>;
    using SystemTestResetListener = ReaderListenerT<CABINA_IDL::st_SystemTestResetIn>;
 
    std::unique_ptr<CBSystems1Listener> cbSystems1Listener;
    std::unique_ptr<BreakerChangeListener> breakerChangeListener;
    std::unique_ptr<AvionicsSwListener> avionicsSwListener;
    std::unique_ptr<SystemTestResetListener> systemTestResetListener;
};