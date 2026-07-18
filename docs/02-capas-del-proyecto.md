# 2. Las capas del proyecto

Este proyecto no es un solo programa: son **cuatro piezas** que colaboran, cada
una en su lugar. Entender qué hace cada capa —y dónde vive— es la mitad del
trabajo.

## Vista general

```
   [ Tu PC ]                    [ Raspberry Pi 4/5 ]                 [ ESP32 ]
      |                                  |                              |
      |   SSH (2.4)                      |                              |
      |--------------------------------> |                              |
      |                          ┌───────┴────────┐                     |
      |                          │  Container     │   canal DDS (2.3)   |
      |                          │  ROS 2 Jazzy   │ <=================> │ Firmware
      |                          │  + micro-ROS   │   USB serial / UDP  │ RTOS (2.1)
      |                          │  Agent  (2.2)  │                     │
      |                          └────────────────┘                     |
```

- **2.1 Firmware RTOS** → corre *dentro* del ESP32.
- **2.2 Nodo edge ROS 2** → corre en la Raspberry Pi, dentro del container.
- **2.3 Canal DDS** → el "idioma" con el que se hablan el ESP32 y la Raspberry.
- **2.4 SSH** → cómo tú, desde tu PC, controlas todo esto a distancia.

Léelas en orde, de abajo hacia arriba (del hardware al humano).

---

## 2.1 Firmware RTOS en el ESP32

**Dónde vive:** grabado en la memoria flash del ESP32. Carpeta `esp32/`.

El ESP32 es un microcontrolador: no tiene Ubuntu ni disco duro, solo un programa
que tú le grabas ("flashear"). Ese programa es el **firmware**.

Aquí usamos **ESP-IDF** (el framework oficial de Espressif) sobre **FreeRTOS**,
un sistema operativo de tiempo real (RTOS). "Tiempo real" significa que las
tareas se ejecutan con temporización predecible —clave en robótica, donde un
retraso puede significar que el robot choque.

### ¿Qué es una tarea de FreeRTOS?

FreeRTOS permite dividir el programa en **tareas** que corren "en paralelo",
turnándose el procesador según su prioridad. En nuestro firmware:

- `app_main()` es el punto de entrada. Lo único que hace es **crear una tarea**.
- `micro_ros_task` es esa tarea: contiene el nodo micro-ROS, el suscriptor y el
  *executor*, y se queda en un bucle escuchando mensajes.
- El `subscription_callback()` se dispara cada vez que llega un mensaje y
  enciende o apaga el LED.

### ¿Qué es micro-ROS?

ROS 2 completo es demasiado pesado para un microcontrolador. **micro-ROS** es la
versión reducida que sí cabe en un ESP32, y le permite comportarse como un
**nodo ROS 2 de verdad**: publicar y suscribirse a topics, igual que un nodo en
una computadora grande.

En nuestro ejemplo, el ESP32 se **suscribe** al topic `led_cmd` y actúa como un
LED controlado por ROS.

> Resumen: el ESP32 corre un pequeño OS de tiempo real (FreeRTOS) y, gracias a
> micro-ROS, se vuelve un nodo del ecosistema ROS 2.

---

## 2.2 Nodo edge con ROS 2 en la Raspberry Pi

**Dónde vive:** en la Raspberry Pi, dentro del container de `.docker/`.

"**Edge**" (borde) significa que el cómputo pasa cerca del sensor/actuador, no en
la nube. La Raspberry Pi es nuestro nodo edge: tiene un Ubuntu 24.04 real y
suficiente potencia para correr **ROS 2 Jazzy completo**.

Su pieza clave para este proyecto es el **micro-ROS Agent**. El ESP32 (con
micro-ROS) no puede hablar directo con la red ROS 2; necesita un **traductor**.
Ese traductor es el Agent:

```
ESP32 (micro-ROS) <--- serial/UDP ---> micro-ROS Agent <--- DDS ---> resto de ROS 2
```

El Agent recibe lo que manda el ESP32 y lo **inyecta en la red ROS 2** como si
fuera un nodo más. Desde la Raspberry, tú ves al ESP32 con comandos normales:

```bash
robot dev              # entrar al container
ros2 node list         # -> /esp32_led_node   (¡el ESP32 aparece!)
ros2 topic list        # -> /led_cmd
ros2 topic pub /led_cmd std_msgs/msg/Bool "{data: true}"   # enciende el LED
```

> Resumen: la Raspberry Pi corre ROS 2 completo dentro de un container, y el
> micro-ROS Agent es el puente que integra al ESP32 en esa red.

---

## 2.3 El canal DDS

**Dónde vive:** es el protocolo de comunicación, no un archivo. Viaja por la red.

**DDS** (Data Distribution Service) es el sistema de mensajería sobre el que está
construido ROS 2. Es lo que hace que un nodo pueda mandar datos a otro sin
saber siquiera dónde está.

Su modelo es **publicar/suscribir (pub/sub)**:

- Un nodo **publica** en un *topic* (un canal con nombre, ej. `led_cmd`).
- Otro nodo se **suscribe** a ese topic y recibe todo lo que se publique.
- Ninguno necesita conocer la dirección IP del otro: DDS los **descubre**
  automáticamente en la red. Esto se llama *discovery*.

### El detalle importante de nuestra configuración

En el `docker-compose.yml` verás:

```yaml
network_mode: host
```

Esto hace que el container **comparta la red de la Raspberry Pi** en vez de tener
una red aislada. ¿Por qué? Porque el *discovery* de DDS necesita ver la red real
para encontrar a todos los nodos. Sin esto, los nodos no se descubrirían entre sí.

También usamos **CycloneDDS** como implementación (variable
`RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`), porque es estable cuando hay varios
nodos micro-ROS conectados.

En este proyecto, el ESP32 se conecta al Agent por **serial (USB)** o por
**Wi-Fi (UDP)**; de ahí para adentro, ya es DDS puro entre todos los nodos ROS 2.

> Resumen: DDS es el "idioma" pub/sub de ROS 2. Los nodos se descubren solos por
> la red y se comunican por topics, sin conocer direcciones fijas.

---

## 2.4 Conexión por SSH a la PC host

**Dónde vive:** es la forma en que tú, desde tu PC, entras a la Raspberry Pi.
Ver también `doc/remote/ssh.md`.

La Raspberry Pi normalmente no tiene monitor ni teclado ("headless"). La
controlas de forma remota desde tu computadora con **SSH** (Secure Shell): abres
una terminal en tu PC que en realidad está ejecutando comandos *dentro* de la
Raspberry.

```bash
# Desde tu PC, entrar a la Raspberry (ajusta usuario e IP):
ssh usuario@192.168.1.50
```

Una vez dentro, ya estás "en" la Raspberry y puedes lanzar todo:

```bash
robot dev       # arrancar el container de ROS 2
robot agent     # lanzar el micro-ROS Agent
```

### El flujo completo en la práctica

1. Enciendes la Raspberry Pi (conectada a la red).
2. Desde tu PC: `ssh usuario@<ip-de-la-rpi>`.
3. Dentro de la Raspberry: `robot dev` y `robot agent`.
4. Conectas el ESP32 (por USB a la Raspberry) ya flasheado con su firmware.
5. El ESP32 aparece como nodo ROS 2; controlas el LED con `ros2 topic pub`.

Así, una sola PC puede manejar toda la flota de robots por SSH, sin cables de
video ni teclados en cada Raspberry.

> Resumen: SSH es tu control remoto por terminal. Te conecta a la Raspberry
> "headless" para operar el container y el resto del sistema desde tu PC.

---

## Cómo encaja todo

Cuando escribes en tu PC:

```
ros2 topic pub /led_cmd std_msgs/msg/Bool "{data: true}"
```

el viaje del mensaje es:

1. **(2.4)** Tu comando entra a la Raspberry por SSH.
2. **(2.2)** ROS 2 en la Raspberry publica el mensaje en el topic `led_cmd`.
3. **(2.3)** DDS lo transporta y el micro-ROS Agent lo recoge.
4. El Agent lo reenvía por serial/UDP al ESP32.
5. **(2.1)** El `subscription_callback` del firmware recibe `true` y **enciende
   el LED**.

Cuatro capas, un solo LED encendido. Eso es un sistema robótico distribuido.
