#include "el6695_ethercat_bridge/el6695_slave.h"
#include <iostream>

namespace el6695_bridge
{

static constexpr uint32_t VENDOR_ID = 0x00000002;
static constexpr uint32_t PRODUCT_ID = 0x1A273052;

EL6695Slave::EL6695Slave(ec_master_t* master, const rclcpp::Logger& logger)
    : master_(master),
    domain_(nullptr),
    sc_(nullptr),
    domain_pd_(nullptr),
    offset_tx_counter_(0),
    offset_rx_counter_(0),
    logger_(logger)
{
    RCLCPP_INFO(logger_, "EL6695Slave created");
}

bool EL6695Slave::configure()
{
    RCLCPP_INFO(logger_, "Configuring EL6695 slave...");

    domain_ = ecrt_master_create_domain(master_);
    if (!domain_) {
        RCLCPP_ERROR(logger_, "Failed to create EtherCAT domain");
        return false;
    }
    RCLCPP_INFO(logger_, "EtherCAT domain created");

    sc_ = ecrt_master_slave_config(master_, 0, 0, VENDOR_ID, PRODUCT_ID);
    if (!sc_) {
        RCLCPP_ERROR(logger_, "Failed to get slave config (vendor=0x%08X, product=0x%08X)",
        VENDOR_ID, PRODUCT_ID);
        return false;
    }
    RCLCPP_INFO(logger_, "Slave config obtained (vendor=0x%08X, product=0x%08X)",
    VENDOR_ID, PRODUCT_ID);

    //create here the pdo entries for your variables
    static ec_pdo_entry_info_t pdo_entries[] = {
    {0x7000, 0x01, 32}, // in32_t to TwinCAT
    {0x7000, 0x02, 8}, // bool to TwinCAT
    {0x6000, 0x01, 32}, // int32_t from TwinCAT
    {0x6000, 0x02, 8}, // bool from TwinCAT
    };

    //change the number of entries/variables
    static ec_pdo_info_t pdos[] = {
    {0x1608, 2, &pdo_entries[0]}, // RxPDO
    {0x1A08, 2, &pdo_entries[2]}, // TxPDO
    };

    //setup of the sync manager
    static ec_sync_info_t syncs[] = {
    {2, EC_DIR_OUTPUT, 1, &pdos[0], EC_WD_DISABLE},
    {3, EC_DIR_INPUT, 1, &pdos[1], EC_WD_DISABLE},
    {0xff, EC_DIR_INVALID, 0, nullptr, EC_WD_DEFAULT}
    };

    RCLCPP_INFO(logger_, "Configuring PDOs...");
    if (ecrt_slave_config_pdos(sc_, EC_END, syncs)) {
        RCLCPP_ERROR(logger_, "Failed to configure PDOs");
        return false;
    }
    RCLCPP_INFO(logger_, "PDOs configured successfully");

    // the run time registration table with the offset
    static ec_pdo_entry_reg_t domain_regs[] = {
    //Tx (Slave->Master)
    {0, 0, VENDOR_ID, PRODUCT_ID, 0x7000, 0x01, &offset_tx_counter_, 0},  // int32_t
    {0, 0, VENDOR_ID, PRODUCT_ID, 0x7000, 0x02, &offset_tx_bool_, 0}, // bool (8-bit)

    //RX (Master -> Slave)
    {0, 0, VENDOR_ID, PRODUCT_ID, 0x6000, 0x01, &offset_rx_counter_, 0}, //int32_t
    {0, 0, VENDOR_ID, PRODUCT_ID, 0x6000, 0x02, &offset_rx_bool_, 0}, // bool (8-bit)
    {0, 0, 0, 0, 0, 0, nullptr, 0}
    };

    RCLCPP_INFO(logger_, "Registering PDO entries into domain...");
        if (ecrt_domain_reg_pdo_entry_list(domain_, domain_regs)) {
        RCLCPP_ERROR(logger_, "Failed to register PDO entry list");
        return false;
    }
    RCLCPP_INFO(logger_, "PDO entries registered (tx offset=%u, rx offset=%u)",
    offset_tx_counter_, offset_rx_counter_);

    RCLCPP_INFO(logger_, "Activating EtherCAT master...");
        if (ecrt_master_activate(master_)) {
        RCLCPP_ERROR(logger_, "Failed to activate EtherCAT master");
        return false;
    }
    RCLCPP_INFO(logger_, "EtherCAT master activated");

    domain_pd_ = ecrt_domain_data(domain_);
    if (!domain_pd_) {
        RCLCPP_ERROR(logger_, "Failed to get domain process data pointer");
        return false;
    }
    RCLCPP_INFO(logger_, "Domain process data pointer obtained — el6695 ready");

    return true;
}

void EL6695Slave::process()
{
    ecrt_master_receive(master_);
    ecrt_domain_process(domain_);

    std::memcpy(&rx_.counter, domain_pd_ + offset_rx_counter_, sizeof(rx_.counter));
    std::memcpy(domain_pd_ + offset_tx_counter_, &tx_.counter, sizeof(tx_.counter));
    std::memcpy(&rx_.flag, domain_pd_ + offset_rx_bool_, sizeof(rx_.flag));
    std::memcpy(domain_pd_ + offset_tx_bool_, &tx_.flag, sizeof(tx_.flag));


    ecrt_domain_queue(domain_);
    ecrt_master_send(master_);
}

void EL6695Slave::setCommand(const ToTwinCAT& cmd)
{
    tx_ = cmd;
}

FromTwinCAT EL6695Slave::getState() const
{
    return rx_;
}

} // namespace el6695_bridge