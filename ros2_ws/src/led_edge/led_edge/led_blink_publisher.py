#!/usr/bin/env python3
"""Nodo edge de ejemplo: hace parpadear el LED del ESP32 publicando en /led_cmd.

Este nodo corre en la Raspberry Pi (dentro del container). Publica mensajes
booleanos alternados en el topic `led_cmd`, al que el ESP32 (micro-ROS) esta
suscrito. Reemplaza al comando manual `ros2 topic pub`.

Ejecutar:
    ros2 run led_edge blink
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import Bool


class LedBlinkPublisher(Node):
    """Publica True/False alternado en /led_cmd a 1 Hz."""

    def __init__(self):
        super().__init__('led_edge_node')
        self.publisher_ = self.create_publisher(Bool, 'led_cmd', 10)
        self.timer = self.create_timer(1.0, self.timer_callback)  # 1 Hz
        self.state = False
        self.get_logger().info('led_edge_node listo, publicando en /led_cmd')

    def timer_callback(self):
        """Alterna el estado y publica el mensaje."""
        self.state = not self.state
        msg = Bool()
        msg.data = self.state
        self.publisher_.publish(msg)
        self.get_logger().info(f'Publicado: {"ON" if self.state else "OFF"}')


def main(args=None):
    rclpy.init(args=args)
    node = LedBlinkPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
