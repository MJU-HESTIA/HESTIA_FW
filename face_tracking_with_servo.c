#include "who_camera.h"
#include "who_human_face_detection.hpp"
#include "driver/mcpwm.h"
#include "soc/mcpwm_periph.h"
#include "esp_log.h"

static QueueHandle_t xQueueAIFrame = NULL;
static const char *TAG = "face_tracking_with_servo";

// SG90 서보모터 핀 번호
#define SERVO_PIN_X 12  // 서보모터 X축 (좌우 움직임)
#define SERVO_PIN_Y 13  // 서보모터 Y축 (상하 움직임)

// 서보모터 초기화 함수
void init_servo()
{
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SERVO_PIN_X);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, SERVO_PIN_Y);

    mcpwm_config_t pwm_config;
    pwm_config.frequency = 50;    // 주파수 = 50Hz, 서보모터는 50Hz PWM 신호 사용
    pwm_config.cmpr_a = 0;        // 듀티 사이클 초기화 (0%)
    pwm_config.cmpr_b = 0;        // 듀티 사이클 초기화 (0%)
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
}

// 서보모터 각도 설정 함수
void set_servo_angle(mcpwm_unit_t mcpwm_num, mcpwm_timer_t timer_num, mcpwm_generator_t gen, float angle)
{
    float duty_us = (angle / 180.0f) * (2400.0f - 600.0f) + 600.0f;  // 서보모터의 펄스 폭 계산
    float duty_cycle = (duty_us / 20000.0f) * 100.0f;  // 듀티 사이클 계산 (20ms 주기 기준)
    mcpwm_set_duty(mcpwm_num, timer_num, gen, duty_cycle);
    mcpwm_set_duty_type(mcpwm_num, timer_num, gen, MCPWM_DUTY_MODE_0);
}

// 얼굴 추적 로직
void track_face(human_face_detection_result_t *result)
{
    static float x_angle = 90.0f;  // 초기 각도 (정면)
    static float y_angle = 90.0f;

    if (result->face_id >= 0) // 얼굴이 감지된 경우
    {
        int face_x = result->box[0] + (result->box[2] - result->box[0]) / 2; // 얼굴 중심 X 좌표
        int face_y = result->box[1] + (result->box[3] - result->box[1]) / 2; // 얼굴 중심 Y 좌표

        int frame_center_x = 160; // QVGA 해상도 기준 중심 X 좌표
        int frame_center_y = 120; // QVGA 해상도 기준 중심 Y 좌표

        // 얼굴이 중심에서 벗어났을 때 각도를 조정
        if (face_x < frame_center_x - 10)
            x_angle += 2.0f; // 좌측으로 이동
        else if (face_x > frame_center_x + 10)
            x_angle -= 2.0f; // 우측으로 이동

        if (face_y < frame_center_y - 10)
            y_angle += 2.0f; // 아래로 이동
        else if (face_y > frame_center_y + 10)
            y_angle -= 2.0f; // 위로 이동

        // 각도 제한 (0도 ~ 180도)
        if (x_angle > 180.0f) x_angle = 180.0f;
        if (x_angle < 0.0f) x_angle = 0.0f;
        if (y_angle > 180.0f) y_angle = 180.0f;
        if (y_angle < 0.0f) y_angle = 0.0f;

        set_servo_angle(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, x_angle);
        set_servo_angle(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, y_angle);
    }
}

// 메인 함수
extern "C" void app_main()
{
    // 서보모터 초기화
    init_servo();

    // 카메라 및 얼굴 인식 초기화
    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);
    register_human_face_detection(xQueueAIFrame, NULL, track_face, NULL, true);

    ESP_LOGI(TAG, "Face tracking with servo started");
}
