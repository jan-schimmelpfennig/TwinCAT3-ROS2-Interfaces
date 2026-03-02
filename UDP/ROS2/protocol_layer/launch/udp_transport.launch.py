from launch import LaunchDescription
from launch_ros.actions import Node, LifecycleNode
from launch.actions import DeclareLaunchArgument, EmitEvent, RegisterEventHandler, LogInfo
from launch.substitutions import LaunchConfiguration, LocalSubstitution
from launch.event_handlers import OnShutdown
from launch.events import matches_action, Shutdown
from launch_ros.events.lifecycle import ChangeState
from lifecycle_msgs.msg import Transition


def generate_launch_description():
    # Declare launch arguments
    ros_address_arg = DeclareLaunchArgument(
        'ros_receiver_ip',
        default_value='50.50.50.1',
        description='IPv4 address of the device that is running ROS.'
    )

    ros_port_arg = DeclareLaunchArgument(
        'ros_receiver_port',
        default_value='11000',
        description='Port of the device that is running ROS'
    )

    twincat_ip_arg = DeclareLaunchArgument(
        'twincat_cx_ip',
        default_value='50.50.50.50',
        description='IPv4 address of the receiver, likely CX (the device that runs TwinCAT)'
    )

    twincat_port_arg = DeclareLaunchArgument(
        'twincat_cx_port',
        default_value='10000',
        description='Port number of the receiver, likely CX (the device that runs TwinCAT)'
    )

    # UDP Protocol Node
    protocol_node = Node(
        package='protocol_layer',
        executable='udp_protocol_node',
        name='udp_protocol_node',
        output='screen',
        parameters=[{
            'use_sim_time': False,
        }]
    )

    # UDP Sender lifecycle Node (from udp_driver package)
    udp_sender_node = LifecycleNode(
        package='udp_driver',
        executable='udp_sender_node_exe',
        name='udp_sender_node',
        namespace='',
        output='screen',
        parameters=[{
            'ip': LaunchConfiguration('twincat_cx_ip'),
            'port': LaunchConfiguration('twincat_cx_port'),
        }]
    )

    # UDP Receiver Node (from udp_driver package)
    udp_receiver_node = LifecycleNode(
        package='udp_driver',
        executable='udp_receiver_node_exe',
        name='udp_receiver_node',
        namespace='',
        output='screen',
        parameters=[{
            'ip': LaunchConfiguration('ros_receiver_ip'),
            'port': LaunchConfiguration('ros_receiver_port'),
        }]
    )

    #### Receiver and Sender are "lifecycle" nodes (like state-machines), so we have
    # to initialize and activate them first, before they work.
    # Configure the UDP sender node after initialization
    configure_udp_sender_node = EmitEvent(
        event=ChangeState(
            lifecycle_node_matcher=matches_action(udp_sender_node),
            transition_id=Transition.TRANSITION_CONFIGURE,
        )
    )

    # Activate the UDP sender node after configuration
    activate_udp_sender_node = EmitEvent(
        event=ChangeState(
            lifecycle_node_matcher=matches_action(udp_sender_node),
            transition_id=Transition.TRANSITION_ACTIVATE,
        )
    )

    # Configure the UDP receiver node
    configure_udp_receiver_node = EmitEvent(
        event=ChangeState(
            lifecycle_node_matcher=matches_action(udp_receiver_node),
            transition_id=Transition.TRANSITION_CONFIGURE,
        )
    )

    # Activate the UDP receiver node
    activate_udp_receiver_node = EmitEvent(
        event=ChangeState(
            lifecycle_node_matcher=matches_action(udp_receiver_node),
            transition_id=Transition.TRANSITION_ACTIVATE,
        )
    )

    # Log shutdown events
    shutdown_nodes = RegisterEventHandler(
        OnShutdown(
            on_shutdown=[LogInfo(
                msg=['Launch was asked to shutdown: ',
                     LocalSubstitution('event.reason')]
            )]
        ),
    )

    return LaunchDescription([
        ros_address_arg,
        ros_port_arg,
        twincat_ip_arg,
        twincat_port_arg,
        protocol_node,
        udp_sender_node,
        udp_receiver_node,
        configure_udp_sender_node,
        activate_udp_sender_node,
        configure_udp_receiver_node,
        activate_udp_receiver_node,
        shutdown_nodes,
    ])