# ESP32 micro-ROS :: LED Listener (ESP-IDF v6.0 + FreeRTOS)

Firmware **nativo ESP-IDF** (no Arduino) que implementa un nodo micro-ROS con
**FreeRTOS**. La ESP32 se suscribe al topic ROS 2 `led_cmd` (`std_msgs/Bool`) y
enciende/apaga el LED integrado. Trabaja contra el **micro-ROS Agent** del
contenedor de la Raspberry Pi definido en `../.docker/`.

## Estructura
```
esp32/
  CMakeLists.txt              # proyecto ESP-IDF
  main/
    CMakeLists.txt            # componente de la app
    app_main.c                # nodo + tarea FreeRTOS + callback (Doxygen)
    Kconfig.projbuild         # opcion de menuconfig: GPIO del LED
  components/                 # aqui se clona el componente micro-ROS
  colcon.meta                 # config de rmw (transporte custom/serial)
  sdkconfig.defaults          # transporte serial UART por defecto
  Doxyfile                    # generacion de docs Doxygen
  README.md
```

## 1. Requisitos: ESP-IDF v6.0

```bash
# Instalar ESP-IDF v6.0 (rama release/v6.0)
mkdir -p ~/esp && cd ~/esp
git clone -b release/v6.0 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32,esp32s3
. ./export.sh    # sourcear en cada terminal
```

## 2. Añadir el componente micro-ROS

Desde la carpeta `esp32/`:

```bash
git clone -b jazzy https://github.com/micro-ROS/micro_ros_espidf_component.git \
    components/micro_ros_espidf_component

# Dependencias Python dentro del venv de IDF
pip install catkin_pkg lark-parser colcon-common-extensions
```

> El componente esta validado oficialmente hasta ESP-IDF v5.5. En v6.0 la
> estructura de build es la misma; si aparece algun warning de version, se
> compila igual. Ante problemas, usar el contenedor `microros/esp-idf-microros`.

## 3. Compilar y flashear

### ESP32 clásica (CP2102/CH340 → `/dev/ttyUSB0`)
```bash
idf.py set-target esp32
idf.py menuconfig      # LED Listener Configuration -> GPIO (default 2)
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### ESP32-S3 (USB nativo → `/dev/ttyACM0`, LED en GPIO48)
```bash
idf.py set-target esp32s3
idf.py menuconfig      # LED Listener Configuration -> GPIO = 48
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

## 4. Probar contra la Raspberry Pi

```bash
# En la RPi:
robot dev
robot agent                     # micro-ROS Agent serial (o 'robot agent udp4 8888')

# En un shell del contenedor:
ros2 node list                  # -> /esp32_led_node
ros2 topic list                 # -> /led_cmd

ros2 topic pub /led_cmd std_msgs/msg/Bool "{data: true}"    # LED ON
ros2 topic pub /led_cmd std_msgs/msg/Bool "{data: false}"   # LED OFF
ros2 topic pub -r 2 /led_cmd std_msgs/msg/Bool "{data: true}"   # blink 2 Hz
```

## 5. Generar la documentación (Doxygen)

```bash
sudo apt-get install -y doxygen graphviz
cd esp32
doxygen Doxyfile
# Abrir docs/html/index.html
```

## Arquitectura FreeRTOS

`app_main()` crea una tarea (`micro_ros_task`) con 16 KB de pila que posee todas
las entidades micro-ROS (nodo, suscriptor, executor) y ejecuta el spin. El
callback `subscription_callback()` corre en el contexto del executor y actualiza
el GPIO del LED. Ante fallo critico de inicializacion, `error_loop()` parpadea el
LED rapidamente como indicador visual.

## Pines LED
| Placa            | Target   | GPIO |
|------------------|----------|------|
| ESP32 DevKit     | esp32    | 2    |
| ESP32-S3 DevKitC | esp32s3  | 48   |
