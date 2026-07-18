# Infra Docker (ROS 2 Jazzy + micro-ROS) para Raspberry Pi 4/5

Despliegue de ROS 2 Jazzy contenedorizado sobre **Raspberry Pi 4 y 5 con Ubuntu 24.04 (arm64)**,
con el **micro-ROS Agent** integrado en la imagen para conectar una **ESP32** como nodo micro-ROS
(por serial USB o por Wi-Fi/UDP).

## 1. Requisitos en la Raspberry Pi

```bash
# Docker Engine + plugin compose
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER      # re-loguear después
sudo apt-get install -y docker-compose-plugin
```

Verifica arquitectura (debe ser arm64):

```bash
uname -m        # aarch64
```

## 2. Estructura

```
.docker/
  rpi/
    Dockerfile           # ROS 2 Jazzy + micro-ROS Agent (compilado en build)
    docker-compose.yml   # network_mode host, privileged, mapeo de /dev
  scripts/
    .config              # nombres de contenedor/imagen/subdir
    robot                # dispatcher: dev | agent | rm
    dev                  # build (1a vez) + arranque + shell ROS 2
    agent                # lanza micro-ROS Agent (serial o udp4)
    rm                   # limpia contenedor e imagen
ros2_ws/                 # workspace de aplicación (montado en el container)
  src/
```

## 3. Uso

Añade los scripts al PATH o llámalos por ruta:

```bash
export PATH="$PWD/.docker/scripts:$PATH"
```

### Arrancar / entrar al contenedor
```bash
robot dev
```
La primera vez compila la imagen **incluyendo el micro-ROS Agent** (tarda varios minutos en la RPi).

### Compilar el workspace (dentro del shell)
```bash
cd /ros2_ws
colcon build --symlink-install
source install/setup.bash
```

## 4. Conectar la ESP32 (micro-ROS)

### Opción A — Serial (USB)
La ESP32 debe estar flasheada con firmware micro-ROS usando **transporte serial**.

1. Conecta la ESP32 por USB. Identifica el puerto en el host:
   ```bash
   ls /dev/ttyUSB* /dev/ttyACM*
   dmesg | grep -i tty
   ```
   - CP2102 / CH340  → `/dev/ttyUSB0`
   - ESP32-S3 (USB nativo) → `/dev/ttyACM0`
2. Si tu puerto no es `/dev/ttyUSB0`, ajústalo en `docker-compose.yml` (sección `devices`).
3. Lanza el agente (en otra terminal, con el container ya corriendo):
   ```bash
   robot agent                       # serial /dev/ttyUSB0 @ 115200
   robot agent serial /dev/ttyACM0   # otro puerto
   robot agent serial /dev/ttyUSB0 921600
   ```

### Opción B — Wi-Fi (UDP)
La ESP32 debe estar flasheada con transporte **UDP** apuntando a la IP de la RPi (puerto 8888).
Como el contenedor usa `network_mode: host`, comparte la LAN directamente:

```bash
robot agent udp4 8888
```

## 5. Verificar el nodo ESP32

Con el agente corriendo y la ESP32 conectada, en un shell del container:

```bash
robot dev
ros2 node list      # deberías ver el nodo publicado desde la ESP32
ros2 topic list
ros2 topic echo /<topic_de_la_esp32>
```

## 6. Limpieza
```bash
robot rm
```

## Notas
- `network_mode: host` + `ipc: host` dan descubrimiento DDS transparente en la LAN.
- `privileged: true` y `/dev` montado permiten acceso a GPIO/I2C/serial y hotplug USB.
- RMW por defecto: **CycloneDDS** (más estable en redes con múltiples nodos micro-ROS).
- Para varias ESP32 por serial simultáneas, duplica el mapeo en `devices` y lanza un agente por puerto.
