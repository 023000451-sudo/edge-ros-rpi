# ROS 2 Jazzy + micro-ROS Agent, base para Raspberry Pi 4/5 (arm64) Ubuntu 24.04
FROM ros:jazzy-ros-base

ENV DEBIAN_FRONTEND=noninteractive

# Core dev tools + deps para compilar micro-ROS Agent
RUN apt-get update && apt-get install -y \
    tmux \
    git \
    nano \
    usbutils \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-msgpack \
    ros-jazzy-rmw-cyclonedds-cpp \
    && rm -rf /var/lib/apt/lists/*

# rosdep
RUN rosdep update

# --- micro-ROS Agent ---
# Workspace dedicado para construir el agente una sola vez en la imagen
WORKDIR /uros_ws
RUN mkdir -p src && cd src && \
    git clone -b jazzy https://github.com/micro-ROS/micro_ros_setup.git

SHELL ["/bin/bash", "-c"]

RUN source /opt/ros/jazzy/setup.bash && \
    apt-get update && \
    rosdep install --from-paths src --ignore-src -y && \
    colcon build && \
    source install/local_setup.bash && \
    ros2 run micro_ros_setup create_agent_ws.sh && \
    ros2 run micro_ros_setup build_agent.sh && \
    rm -rf /var/lib/apt/lists/*

# Workspace de aplicación (ros2 edge node)
WORKDIR /ros2_ws

# Entorno ROS siempre disponible
RUN echo "source /opt/ros/jazzy/setup.bash" >> /root/.bashrc && \
    echo "[ -f /uros_ws/install/local_setup.bash ] && source /uros_ws/install/local_setup.bash" >> /root/.bashrc && \
    echo "[ -f /ros2_ws/install/setup.bash ] && source /ros2_ws/install/setup.bash" >> /root/.bashrc

CMD ["bash"]
