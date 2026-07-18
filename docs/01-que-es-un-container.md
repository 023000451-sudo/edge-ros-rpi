# 1. ¿Qué es un container?

## La idea en una frase

Un **container** es una caja aislada donde vive un programa junto con *todo* lo
que necesita para funcionar: sus librerías, sus versiones exactas, sus variables
de entorno. Esa caja corre igual en cualquier computadora que tenga Docker.

## El problema que resuelve

Seguro han oído (o dicho) la frase clásica:

> "En mi máquina sí funciona."

Pasa porque ROS 2 necesita una versión concreta de Ubuntu, ciertas librerías,
cierto middleware... y cada quien tiene su laptop distinta. Instalar todo eso a
mano en la Raspberry Pi, sin equivocarse, es frágil y se rompe fácil.

Un container empaqueta ROS 2 Jazzy + el micro-ROS Agent ya configurados. En vez
de instalar 20 cosas en la Raspberry, corres **una** caja que ya trae todo
adentro y siempre arranca igual.

## Container ≠ Máquina virtual

Es la confusión más común. La diferencia importa:

| | Máquina virtual (VM) | Container |
|---|---|---|
| Qué virtualiza | Una computadora completa (con su propio sistema operativo) | Solo el programa y sus dependencias |
| Peso | Pesada (GB, minutos en arrancar) | Ligera (MB, segundos en arrancar) |
| Kernel | Trae el suyo propio | Comparte el del sistema anfitrión |

Analogía: una **VM** es como construir una casa entera dentro de otra casa. Un
**container** es como una habitación amueblada y lista para usar: comparte los
cimientos de la casa, pero está aislada del resto.

## Vocabulario mínimo

- **Imagen (image):** la "receta" congelada. Es lo que se construye una vez con
  el `Dockerfile`. Piensa en el molde.
- **Container:** una instancia viva de esa imagen, ejecutándose. Del molde
  salen los panes.
- **Dockerfile:** el archivo de texto que describe cómo construir la imagen
  (qué base usar, qué instalar). Está en `.docker/rpi/Dockerfile`.
- **docker compose:** una forma de describir *cómo se ejecuta* el container
  (puertos, dispositivos, permisos) en un archivo `docker-compose.yml`, para no
  escribir comandos larguísimos a mano.
- **Volumen (volume):** una carpeta de la Raspberry Pi que se "mapea" dentro del
  container, para que los archivos sobrevivan aunque el container se borre.

## Cómo lo usamos en este proyecto

Toda la infra está en la carpeta `.docker/`. Con los scripts de ayuda no
necesitas memorizar comandos de Docker:

```bash
robot dev     # construye (solo la 1a vez) y te mete a la terminal de ROS 2
robot agent   # lanza el micro-ROS Agent que habla con el ESP32
robot rm      # borra el container y libera espacio
```

La **primera vez** que corres `robot dev`, Docker construye la imagen: descarga
ROS 2 Jazzy y compila el micro-ROS Agent. Tarda varios minutos. A partir de ahí,
arrancar es cuestión de segundos porque la imagen ya está construida.

## Lo que tienes que recordar

- Un container = tu programa + sus dependencias, aislado y reproducible.
- No es una máquina virtual: es mucho más ligero.
- En este proyecto, el container vive en la **Raspberry Pi** y contiene **ROS 2
  Jazzy + el micro-ROS Agent**.
- El firmware del ESP32 **no** va en el container: eso se compila aparte con
  ESP-IDF en tu PC.
