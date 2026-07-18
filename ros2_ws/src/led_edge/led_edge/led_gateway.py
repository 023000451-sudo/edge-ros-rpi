#!/usr/bin/env python3
"""Nodo edge de ejemplo: el usuario controla el parpadeo del LED del ESP32 publicando en /led_cmd.

Este nodo corre en la Raspberry Pi (dentro del container). Publica mensajes
booleanos en el topic `led_cmd`, al que el ESP32 (micro-ROS) esta
suscrito. Reemplaza al comando manual `ros2 topic pub --once`.

Ejecutar:
    ros2 run led_edge blink
"""

import rclpy
from rclpy.node import Node
from std_msgs.msg import Bool
from std_msgs.msg import String

class LedGateway(Node):
    """Publica True/False alternado en /led_cmd a 1 Hz."""

    def __init__(self):
        
        super().__init__('led_edge_node')
        
        self.publisher_ = self.create_publisher(Bool, 'led_cmd', 10)
        
        # creamos subscriptions por cuantos compañeros haya
        self.sub1 = self.create_subscription(String, 'yari', self.listener_callback, 10)
        self.sub2 = self.create_subscription(String, 'rodriguez', self.listener_callback, 10)
        
        self.sub1 # Prevent unused variable warning
        self.sub2 # Prevent unused variable warning
        
        self.state = False # Led state
        self.get_logger().info('led_edge_node listo, publicando en /led_cmd')

    def listener_callback(self, msg):
        """Alterna el estado y publica el mensaje."""
        # Imprimimos mensaje que se escucho
        self.get_logger().info(f'Recibido: "{msg}"')
        # Convertimos el mensaje String -> Bool  
        self.state = not self.state
        esp32_msg = Bool()
        esp32_msg.data = self.state
        # Publicamos el mensaje Bool al ESP32
        self.publisher_.publish(esp32_msg)
        self.get_logger().info(f'Publicado: {"ON" if esp32_msg.data else "OFF"}')


def main(args=None):
    rclpy.init(args=args)
    node = LedGateway()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()