# Documentación del proyecto — micro-ROS Edge

Guía de lectura para alumnos. Empieza por el concepto y baja al detalle.

1. [¿Qué es un container?](01-que-es-un-container.md)
   Idea base de contenedores y por qué los usamos en la Raspberry Pi.

2. [Las capas del proyecto](02-capas-del-proyecto.md)
   Cómo encajan el ESP32, la Raspberry Pi, DDS y tu PC:
   - 2.1 Firmware RTOS en el ESP32
   - 2.2 Nodo edge con ROS 2 en la Raspberry Pi
   - 2.3 El canal DDS
   - 2.4 Conexión por SSH desde tu PC

3. [El workspace `ros2_ws` y VS Code](03-workspace-y-vscode.md)
   Qué código va en `ros2_ws`, y cómo editarlo desde VS Code (Remote-SSH) para
   que el container lo compile y ejecute.

> Requisito previo: repo clonado y la infra de `.docker/` en la Raspberry Pi.
> Ver `doc/docker/setup.md`, `doc/remote/ssh.md` y `doc/remote/vscode.md`.
