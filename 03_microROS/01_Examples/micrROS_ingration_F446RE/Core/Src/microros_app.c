/*
 * microro_app.c
 *
 *  Created on: Oct 10, 2022
 *      Author: alejo
 */

#include "microros_app.h"

#define TELEMETRY_TOPIC "/control/telemetry"
#define LED_TOPIC "/control/led"
#define NODE_NAME  "telemetry_node"

rcl_publisher_t telemetry_pub;
rcl_subscription_t led_enable_sub;
rcl_timer_t pub_timer;


sensor_telemetry__msg__Telemetry outcomming_telemetry;
std_msgs__msg__Bool inconming_led;


void led_subscription_callback(const void* data) {
	const std_msgs__msg__Bool* msg = (std_msgs__msg__Bool*)data;

	if (msg->data == 1) {
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
	} else {
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	}
	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
}

void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
	RCLC_UNUSED(last_call_time);

	outcomming_telemetry.temperature += 0.5;
	outcomming_telemetry.humidity += 0.5;

	if (timer != NULL) {
		rcl_publish(&telemetry_pub, &outcomming_telemetry, NULL);
	}
}


rcl_node_t node;


void init_uros() {

	rmw_uros_set_custom_transport(
	true,
	(void *) &huart2,
	transport_serial_open,
	transport_serial_close,
	transport_serial_write,
	transport_serial_read);


	  // Launch app thread when IP configured
	  rcl_allocator_t freeRTOS_allocator = rcutils_get_zero_initialized_allocator();
	  freeRTOS_allocator.allocate = __freertos_allocate;
	  freeRTOS_allocator.deallocate = __freertos_deallocate;
	  freeRTOS_allocator.reallocate = __freertos_reallocate;
	  freeRTOS_allocator.zero_allocate = __freertos_zero_allocate;

	  if (!rcutils_set_default_allocator(&freeRTOS_allocator))
	  {
	    printf("Error on default allocators (line %d)\n", __LINE__);
	  }
}


void main_ros_app() {

	init_uros();

	rcl_ret_t ret= 0;

	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;

	/* Init options */
	ret = rclc_support_init(&support, 0, NULL, &allocator);
	rcl_node_options_t node_ops = rcl_node_get_default_options();

	/* Set node ROS domain ID to 10 */
	node_ops.domain_id = (size_t)(10);

	/* Init node with custom options */
	ret = rclc_node_init_with_options(&node, NODE_NAME, "", &support, &node_ops);

	/* Create publisher */
	ret = rclc_publisher_init_default(
			&telemetry_pub,
			&node,
			ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_telemetry, msg, Telemetry),
			TELEMETRY_TOPIC);

	/* Create subscriber */

	ret = rclc_subscription_init_default(
			&led_enable_sub,
			&node,
			ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Bool),
			LED_TOPIC);


	/* Create timer */
	ret = rclc_timer_init_default(&pub_timer, &support, RCL_MS_TO_NS(1000), timer_callback);

	rclc_executor_t executor;

	ret = rclc_executor_init(&executor, &support.context, 3, &allocator);

	ret = rclc_executor_add_subscription(
			&executor,
			&led_enable_sub,
			&inconming_led,
			led_subscription_callback,
			ON_NEW_DATA);

	ret = rclc_executor_add_timer(&executor, &pub_timer);


	while(1){
		rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
		usleep(10000);
	}
}

void create_ros_task() {
	xTaskCreate(
			main_ros_app,
			ROS_TASK_NAME,
			ROS_TASK_STACK,
			NULL,
			ROS_TASK_PRIO,
			NULL);
}
