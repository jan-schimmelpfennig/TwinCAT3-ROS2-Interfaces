#ifndef EL6695_ETHERCAT_BRIDGE_EL6695_SLAVE_H
#define EL6695_ETHERCAT_BRIDGE_EL6695_SLAVE_H

#include <ecrt.h>
#include <cstdint>
#include <cstring>
#include <rclcpp/rclcpp.hpp>

namespace el6695_bridge
{

#pragma pack(push, 1)
    struct ToTwinCAT {
        int32_t counter;
        uint8_t flag;
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct FromTwinCAT {
        int32_t counter;
        uint8_t flag;
    };
#pragma pack(pop)

    class EL6695Slave
    {
    public:
        explicit EL6695Slave(ec_master_t* master, const rclcpp::Logger& logger);
        ~EL6695Slave() = default;

        bool configure();
        void process();

        void setCommand(const ToTwinCAT& cmd);
        [[nodiscard]] FromTwinCAT getState() const;

    private:
        ec_master_t* master_;
        ec_domain_t* domain_;
        ec_slave_config_t* sc_;
        uint8_t* domain_pd_;
        unsigned int offset_tx_counter_;
        unsigned int offset_rx_counter_;
        unsigned int offset_rx_bool_;
        unsigned int offset_tx_bool_;

        ToTwinCAT tx_;
        FromTwinCAT rx_;

        rclcpp::Logger logger_;
    };

} // namespace el6695_bridge
#endif