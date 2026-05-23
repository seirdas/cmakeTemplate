#include "dds/FastDDS.hpp"

#if defined FASTDDS || defined FASTDDS_VERSION

    #include <memory>
    #include <mutex>
    #include <atomic>
    #include <thread>
    #include <functional>

    using namespace eprosima::fastdds::dds;

    CABINA_IDL::st_CircuitBreakersSystems1 CB_Systems1_Out;

    // General ------------------------------------------------------------------------------

    FastDDS::FastDDS() {
        CBSystems1Type = TypeSupport(new CABINA_IDL::st_CircuitBreakersSystems1PubSubType());
        BreakerChangeType = TypeSupport(new CABINA_IDL::st_BreakerChangeEventPubSubType());
        AvionicsSwType = TypeSupport(new CABINA_IDL::st_AvionicsSwitchesInPubSubType());
        SystemTestResetType = TypeSupport(new CABINA_IDL::st_SystemTestResetInPubSubType());
        CBSystems1Type_Out = TypeSupport(new CABINA_IDL::st_CircuitBreakersSystems1PubSubType());
    }

    FastDDS::~FastDDS() {
        if (participant_ != nullptr)
        {
            participant_->delete_contained_entities();
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
            participant_ = nullptr;
        }
    }

    
    void FastDDS::Inicializa_DDS()
    {
        const uint32_t DOMAIN_ID = 0;
    
        const char* TOPIC_CB_SYSTEMS1_IN = "SES/cb_systems1_in";
        const char* TOPIC_BREAKERS_CHANGE_IN       = "SES/breakers_change_in";
        const char* TOPIC_AVIONICS_SW_IN        = "SES/avionics_sw_in";
        const char* TOPIC_SYSTEM_TEST_RESET_IN  = "SES/system_test_reset_in";
        const char* TOPIC_CB_SYSTEMS1_OUT       = "SES/cb_systems1_out";
    
        DomainParticipantQos pqos;
        pqos.name("DDSMonitorParticipant");
    
        participant_ =
            DomainParticipantFactory::get_instance()->create_participant(DOMAIN_ID, pqos);
    
        if (participant_ == nullptr)
        {
            std::cout << "ERROR: No se pudo crear el DomainParticipant" << std::endl;
            return;
        }
    
        CBSystems1Type.register_type(participant_);
        BreakerChangeType.register_type(participant_);
        AvionicsSwType.register_type(participant_);
        SystemTestResetType.register_type(participant_);
        CBSystems1Type_Out.register_type(participant_);
    
        topicCBSystems1In_ = participant_->create_topic(
            TOPIC_CB_SYSTEMS1_IN,
            CBSystems1Type.get_type_name(),
            TOPIC_QOS_DEFAULT);

        topicCBSystems1Out_ = participant_->create_topic(
            TOPIC_CB_SYSTEMS1_OUT,
            CBSystems1Type_Out.get_type_name(),
            TOPIC_QOS_DEFAULT);
    
        topicBreakerChangeIn_ = participant_->create_topic(
            TOPIC_BREAKERS_CHANGE_IN,
            BreakerChangeType.get_type_name(),
            TOPIC_QOS_DEFAULT);
    
        topicAvionicsSwIn_ = participant_->create_topic(
            TOPIC_AVIONICS_SW_IN,
            AvionicsSwType.get_type_name(),
            TOPIC_QOS_DEFAULT);
    
        topicSystemTestResetIn_ = participant_->create_topic(
            TOPIC_SYSTEM_TEST_RESET_IN,
            SystemTestResetType.get_type_name(),
            TOPIC_QOS_DEFAULT);
    
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    
        if (subscriber_ == nullptr)
        {
            std::cout << "ERROR: No se pudo crear el Subscriber" << std::endl;
            return;
        }
    
        DataReaderQos readerQos;
        readerQos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        readerQos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        readerQos.history().kind = KEEP_LAST_HISTORY_QOS;
        readerQos.history().depth = 1;

        DataWriterQos writerQos;
        writerQos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        writerQos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        writerQos.history().kind = KEEP_LAST_HISTORY_QOS;
        writerQos.history().depth = 1;
    
        cbSystems1Listener = std::make_unique<CBSystems1Listener>(
            [this](const CABINA_IDL::st_CircuitBreakersSystems1& msg)
            {
                onCBSystems1InReceived(msg);
            });
    
        breakerChangeListener = std::make_unique<BreakerChangeListener>(
            [this](const CABINA_IDL::st_BreakerChangeEvent& msg)
            {
                onBreakerChangeInReceived(msg);
            });
    
        avionicsSwListener = std::make_unique<AvionicsSwListener>(
            [this](const CABINA_IDL::st_AvionicsSwitchesIn& msg)
            {
                onAvionicsSwInReceived(msg);
            });
    
        systemTestResetListener = std::make_unique<SystemTestResetListener>(
            [this](const CABINA_IDL::st_SystemTestResetIn& msg)
            {
                onSystemTestResetInReceived(msg);
            });
    
        CBSystems1Reader = subscriber_->create_datareader(
            topicCBSystems1In_,
            readerQos,
            cbSystems1Listener.get());
    
        BreakerChangeReader = subscriber_->create_datareader(
            topicBreakerChangeIn_,
            readerQos,
            breakerChangeListener.get());
    
        AvionicsSwReader = subscriber_->create_datareader(
            topicAvionicsSwIn_,
            readerQos,
            avionicsSwListener.get());
    
        SystemTestResetReader = subscriber_->create_datareader(
            topicSystemTestResetIn_,
            readerQos,
            systemTestResetListener.get());

        CBSystems1Writer = publisher_->create_datawriter(topicCBSystems1Out_, writerQos);
    
    
        std::cout << "Monitor DDS iniciado. Esperando mensajes..." << std::endl;
    }
    
    void FastDDS::onCBSystems1InReceived(
        const CABINA_IDL::st_CircuitBreakersSystems1& msg)
    {
        std::cout << std::endl;
        std::cout << "[READ] SES/cb_systems1_in" << std::endl;
        std::cout << "       bkrTrimAil      = " << msg.bkrTrimAil() << std::endl;
        std::cout << "       bkrTrimARTCU   = " << msg.bkrTrimARTCU() << std::endl;
        std::cout << "       bkrTrimStbyRud = " << msg.bkrTrimStbyRud() << std::endl;
    }
    
    void FastDDS::onBreakerChangeInReceived(
        const CABINA_IDL::st_BreakerChangeEvent& msg)
    {
        std::cout << std::endl;
        std::cout << "[READ] SES/breakers_change_in" << std::endl;
        std::cout << "       id           = " << msg.id() << std::endl;
        std::cout << "       panelId      = " << static_cast<int>(msg.panelId()) << std::endl;
        std::cout << "       breakerValue = " << msg.breakerValue() << std::endl;
    }
    
    void FastDDS::onAvionicsSwInReceived(
        const CABINA_IDL::st_AvionicsSwitchesIn& msg)
    {
        std::cout << std::endl;
        std::cout << "[READ] SES/avionics_sw_in" << std::endl;
        std::cout << "       swMHF  = " << msg.data().swMHF << std::endl;
        std::cout << "       swVUHF = " << msg.data().swVUHF << std::endl;
    }
    
    void FastDDS::onSystemTestResetInReceived(
        const CABINA_IDL::st_SystemTestResetIn& msg)
    {
        std::cout << std::endl;
        std::cout << "[READ] SES/system_test_reset_in" << std::endl;
        std::cout << "       pbGMeterReset = " << msg.data().pbGMeterReset << std::endl;
        std::cout << "       pbLdgGrTest   = " << msg.data().pbLdgGrTest << std::endl;
        std::cout << "       pbSWRSTest    = " << msg.data().pbSWRSTest << std::endl;
    }

    void FastDDS::writeCBSystems1()
    {
        CB_Systems1_Out.bkrTrimAil() = 1;
        CB_Systems1_Out.bkrTrimARTCU() = 1;
        CB_Systems1_Out.bkrTrimStbyRud() = 1;
    
        if (CBSystems1Writer != nullptr)
        {
            CBSystems1Writer->write(&CB_Systems1_Out);
        }
    }


#else
// ============================================================
//  (Stubs)
// ============================================================

    // General ------------------------------------------------------------------------------
    FastDDS::FastDDS()  {}
    FastDDS::~FastDDS() {}
#endif
