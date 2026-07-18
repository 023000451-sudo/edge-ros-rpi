from setuptools import find_packages, setup

package_name = 'led_edge'

setup(
    name=package_name,
    version='0.1.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='UPSRJ Robotica',
    maintainer_email='jesus.loport@outlook.com',
    description='Nodo edge de ejemplo: publica en /led_cmd para el ESP32 micro-ROS.',
    license='Apache-2.0',
    entry_points={
        'console_scripts': [
            'led_edge = led_edge.led_edge:main',
            # Uso: ros2 run led_edge blink
            'blink = led_edge.led_blink_publisher:main',
        ],
    },
)
