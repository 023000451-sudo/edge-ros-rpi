/**
 * @file app_main.c
 * @brief Nodo micro-ROS sobre ESP-IDF v6.0 + FreeRTOS: listener de LED.
 *
 * @details
 * Este firmware implementa un nodo micro-ROS que se suscribe al topic ROS 2
 * @c led_cmd (tipo @c std_msgs/msg/Bool) y refleja el valor recibido en el LED
 * integrado de la ESP32. Se comunica con el @b micro-ROS @b Agent que corre en
 * el contenedor Docker de la Raspberry Pi (ver carpeta @c .docker/).
 *
 * La arquitectura sigue el modelo de tareas de FreeRTOS:
 *  - Una tarea dedicada (@ref micro_ros_task) posee todas las entidades
 *    micro-ROS (nodo, suscriptor, executor) y ejecuta el spin del executor.
 *  - El callback @ref subscription_callback actualiza el estado del LED cuando
 *    llega un mensaje.
 *
 * @note Transporte: se compila con transporte UART serial hacia el agente
 *       (configurado en @c colcon.meta). Para Wi-Fi/UDP usar @c menuconfig.
 *
 * @par Prueba desde el contenedor de la RPi
 * @code{.sh}
 *   robot dev
 *   robot agent                 # micro-ROS Agent en serial
 *   ros2 topic pub /led_cmd std_msgs/msg/Bool "{data: true}"
 *   ros2 topic pub -r 2 /led_cmd std_msgs/msg/Bool "{data: true}"
 * @endcode
 *
 * @author UPSRJ - Ingenieria en Robotica
 * @date 2026
 * @copyright Apache-2.0
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/bool.h>

#include <uros_network_interfaces.h>
#include <rmw_microros/rmw_microros.h>

/** @brief Pin del LED integrado. GPIO2 en la mayoria de DevKit ESP32. */
#ifndef LED_GPIO
#define LED_GPIO CONFIG_LED_GPIO
#endif

/** @brief Etiqueta para los logs de ESP-IDF (@c ESP_LOGx). */
static const char *TAG = "esp32_led_listener";

/** @brief Suscriptor micro-ROS al topic @c led_cmd. */
static rcl_subscription_t g_subscriber;

/** @brief Buffer del mensaje entrante (@c std_msgs/Bool). */
static std_msgs__msg__Bool g_msg;

/**
 * @brief Macro de verificacion estricta: entra en panico si @p fn falla.
 * @param fn Llamada a una funcion de la API RCL/RCLC que retorna @c rcl_ret_t.
 * @details Ante un error irrecuperable durante la inicializacion, se registra
 *          el codigo y se reinicia el flujo mediante @ref error_loop.
 */
#define RCCHECK(fn)                                                            \
    {                                                                          \
        rcl_ret_t temp_rc = fn;                                                \
        if (temp_rc != RCL_RET_OK) {                                           \
            ESP_LOGE(TAG, "Fallo en %s, linea %d (rc=%d)", #fn, __LINE__,      \
                     (int)temp_rc);                                            \
            error_loop();                                                      \
        }                                                                      \
    }

/**
 * @brief Macro de verificacion suave: registra el error pero continua.
 * @param fn Llamada a una funcion de la API RCL/RCLC que retorna @c rcl_ret_t.
 */
#define RCSOFTCHECK(fn)                                                        \
    {                                                                          \
        rcl_ret_t temp_rc = fn;                                                \
        if (temp_rc != RCL_RET_OK) {                                           \
            ESP_LOGW(TAG, "Aviso en %s, linea %d (rc=%d)", #fn, __LINE__,      \
                     (int)temp_rc);                                            \
        }                                                                      \
    }

/**
 * @brief Bucle de panico: parpadea el LED rapidamente de forma indefinida.
 *
 * @details Se invoca cuando una operacion critica de micro-ROS falla. El
 *          parpadeo rapido sirve como indicador visual de error en la placa.
 *          Esta funcion no retorna.
 */
static void error_loop(void)
{
    while (true) {
        gpio_set_level(LED_GPIO, !gpio_get_level(LED_GPIO));
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * @brief Callback del suscriptor: actualiza el LED con el valor recibido.
 *
 * @param[in] msgin Puntero opaco al mensaje recibido; se castea a
 *                  @c std_msgs__msg__Bool.
 *
 * @details Si @c data es @c true enciende el LED; si es @c false lo apaga.
 *          El executor invoca esta funcion cada vez que llega un mensaje nuevo.
 */
static void subscription_callback(const void *msgin)
{
    const std_msgs__msg__Bool *msg = (const std_msgs__msg__Bool *)msgin;
    gpio_set_level(LED_GPIO, msg->data ? 1 : 0);
    ESP_LOGI(TAG, "led_cmd recibido: %s", msg->data ? "ON" : "OFF");
}

/**
 * @brief Tarea FreeRTOS que aloja el nodo micro-ROS y ejecuta el executor.
 *
 * @param[in] arg No utilizado (interfaz estandar de tarea FreeRTOS).
 *
 * @details Secuencia de inicializacion:
 *  1. Configura el GPIO del LED como salida.
 *  2. Inicializa el transporte de red micro-ROS.
 *  3. Crea el soporte, el nodo (@c esp32_led_node) y el suscriptor a @c led_cmd.
 *  4. Crea el executor y le agrega el suscriptor con @ref subscription_callback.
 *  5. Entra en un bucle de spin que procesa los mensajes entrantes.
 *
 * @note La tarea no retorna en operacion normal. Si el bucle termina, limpia
 *       las entidades y se autoelimina con @c vTaskDelete.
 */
static void micro_ros_task(void *arg)
{
    (void)arg;

    // 1. GPIO del LED
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);

    // 2. Transporte de red micro-ROS (serial o UDP segun menuconfig)
#if defined(CONFIG_MICRO_ROS_ESP_NETIF_ENABLE) || defined(CONFIG_MICRO_ROS_ESP_WIFI_ENABLE)
    ESP_ERROR_CHECK(uros_network_interface_initialize());
#endif

    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;

    // 3. Soporte + nodo + suscriptor
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

    rcl_node_t node;
    RCCHECK(rclc_node_init_default(&node, "esp32_led_node", "", &support));

    RCCHECK(rclc_subscription_init_default(
        &g_subscriber,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
        "led_cmd"));

    // 4. Executor
    rclc_executor_t executor;
    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
    RCCHECK(rclc_executor_add_subscription(
        &executor, &g_subscriber, &g_msg, &subscription_callback, ON_NEW_DATA));

    ESP_LOGI(TAG, "Nodo micro-ROS listo. Esperando en topic /led_cmd...");

    // 5. Spin
    while (true) {
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Limpieza (no se alcanza en operacion normal)
    RCSOFTCHECK(rcl_subscription_fini(&g_subscriber, &node));
    RCSOFTCHECK(rcl_node_fini(&node));
    vTaskDelete(NULL);
}

/**
 * @brief Punto de entrada de la aplicacion ESP-IDF.
 *
 * @details Crea la tarea @ref micro_ros_task fijada al nucleo de la aplicacion,
 *          con una pila amplia (micro-ROS necesita varios KB). El scheduler de
 *          FreeRTOS ya esta corriendo cuando ESP-IDF invoca @c app_main.
 */
void app_main(void)
{
    xTaskCreate(
        micro_ros_task,
        "micro_ros_task",
        16 * 1024,    // 16 KB de pila
        NULL,
        5,            // prioridad
        NULL);
}
