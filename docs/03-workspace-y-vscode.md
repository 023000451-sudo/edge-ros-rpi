# 3. El workspace `ros2_ws` y cómo editarlo desde VS Code

## Qué es `ros2_ws`

Es el **workspace de ROS 2** (formato colcon) donde vive el código que corre en
la Raspberry Pi: los **nodos edge** que le hablan al ESP32. Estructura:

```
ros2_ws/
  src/                       <- tus paquetes (esto es lo único que versionas)
    led_edge/                   paquete de ejemplo (incluido)
      package.xml               metadatos y dependencias
      setup.py                  cómo se instala / qué ejecutables expone
      led_edge/
        led_blink_publisher.py  el nodo: publica en /led_cmd
  build/  install/  log/     <- los genera 'colcon build' (van en .gitignore)
```

Regla de oro: **solo se versiona `src/`**. Las carpetas `build/`, `install/` y
`log/` son artefactos de compilación y se regeneran; nunca van a git.

### El paquete de ejemplo `led_edge`

Ya viene un paquete listo. Es el "otro lado" del firmware del ESP32: en vez de
encender el LED a mano con `ros2 topic pub`, este nodo publica en `led_cmd`
automáticamente cada segundo, alternando ON/OFF. El ESP32, suscrito a ese topic,
parpadea.

```
[ led_edge en la RPi ]  --publica-->  /led_cmd  --DDS+Agent-->  [ ESP32 LED ]
```

## Cómo editar el código desde VS Code y que el container lo vea

Aquí está la clave que hace todo simple: en el `docker-compose.yml` montamos el
workspace como **bind mount**:

```yaml
volumes:
  - ../../ros2_ws:/ros2_ws
```

Esto significa que `ros2_ws/` en la Raspberry y `/ros2_ws` dentro del container
son **la misma carpeta física**. No hay copia ni sincronización: editas un
archivo y el container lo ve en el mismo instante. Solo falta recompilar.

### El flujo completo (headless, desde tu PC)

La Raspberry no tiene monitor. Usas **VS Code Remote-SSH** para editar sus
archivos como si fueran locales. (Guía de instalación: `doc/remote/vscode.md`.)

**Paso 1 — Conectar VS Code a la Raspberry**
```
Ctrl+Shift+P -> Remote-SSH: Connect to Host -> turtle
```
VS Code abre una ventana que corre *en la Raspberry*. Abres la carpeta del repo.

**Paso 2 — Editar tu nodo**
Modificas, por ejemplo, `ros2_ws/src/led_edge/led_edge/led_blink_publisher.py`
y guardas. El archivo real ya está en la Raspberry, así que el container también
lo tiene (por el bind mount).

**Paso 3 — Compilar y correr dentro del container**
En una terminal (dentro de VS Code, o por SSH):
```bash
robot dev                          # entrar al container
cd /ros2_ws
colcon build --symlink-install     # compilar
source install/setup.bash
ros2 run led_edge blink            # correr tu nodo
```

### El atajo: `--symlink-install`

Con `colcon build --symlink-install`, colcon **enlaza** los archivos Python en
vez de copiarlos. Resultado: después de la primera compilación, editas un `.py`,
guardas, y `ros2 run` ya usa la versión nueva **sin recompilar**.

- **Python:** compilas una vez con `--symlink-install`, luego solo editas y corres.
- **C++:** siempre hay que recompilar (`colcon build`) tras cada cambio.

## Resumen del ciclo de trabajo

```
   VS Code (Remote-SSH)          Raspberry Pi                Container
   editas led_blink_publisher.py                              
        |                                                     
        |  guardar  ->  ros2_ws/src/...  ==(bind mount)==>  /ros2_ws/src/...
        |                                                     |
        |                                          colcon build --symlink-install
        |                                          ros2 run led_edge blink
        v                                                     |
   (repites: editar -> guardar -> ros2 run)                   v
                                                       publica en /led_cmd -> ESP32
```

1. Editas en VS Code (Remote-SSH a la Raspberry).
2. Guardas — el container ya ve el cambio (misma carpeta).
3. `colcon build --symlink-install` una vez; luego `ros2 run`.
4. Tu nodo publica en `/led_cmd` y el ESP32 responde.

## Crear tu propio paquete (referencia rápida)

```bash
robot dev
cd /ros2_ws/src
ros2 pkg create --build-type ament_python mi_paquete \
    --dependencies rclpy std_msgs
# ...editas tu nodo, lo agregas a entry_points en setup.py...
cd /ros2_ws && colcon build --symlink-install
```
