import math
import numpy as np
import matplotlib.pyplot as plt

def draw_arc_with_angles(x, y):
    if y <= 0:
        raise ValueError("y > 0 이어야 합니다. (점은 1,2사분면에 있어야 함)")

    # 반지름 r 계산
    r = (x**2 + y**2) / (2*y)
    cx, cy = 0, r  # 중심 (0, r)

    # 중심으로부터 두 점까지의 각도
    theta_O = math.atan2(0 - cy, 0 - cx)
    theta_P = math.atan2(y - cy, x - cx)

    # 각도 차이를 [-pi, pi]로 조정
    def wrap(a):
        while a >= math.pi: a -= 2*math.pi
        while a < -math.pi: a += 2*math.pi
        return a

    theta_O, theta_P = wrap(theta_O), wrap(theta_P)
    delta = wrap(theta_P - theta_O)

    # 호의 각도 범위 (짧은 쪽 선택)
    thetas = np.linspace(theta_O, theta_O + delta, 300)

    # 호 좌표
    arc_x = cx + r * np.cos(thetas)
    arc_y = cy + r * np.sin(thetas)

    # 전체 원 좌표 (참고용)
    t = np.linspace(0, 2*math.pi, 600)
    circ_x = cx + r * np.cos(t)
    circ_y = cy + r * np.sin(t)

    # 접선 기울기 및 각도 계산
    # 시작점 (0,0)에서의 접선: y = 0
    # 원의 방정식: x^2 + (y-r)^2 = r^2
    # 미분하면 2x + 2(y-r)y' = 0 -> y' = -x / (y-r)
    # 시작점 (0,0)에서의 기울기: y'(0,0) = -0 / (0-r) = 0
    # 시작점 각도 (radian)
    angle_start_rad = math.atan2(-0, 0 - r) + math.pi/2 # 접선은 반지름에 수직
    angle_start_deg = math.degrees(angle_start_rad)

    # 끝점 (x,y)에서의 기울기: y'(x,y) = -x / (y-r)
    # 끝점 각도 (radian)
    angle_end_rad = math.atan2(-x, y - r) + math.pi/2 # 접선은 반지름에 수직
    angle_end_deg = math.degrees(angle_end_rad)

    # 두 각도 차이
    angle_diff_deg = abs(angle_end_deg - angle_start_deg)
    
    # 각도를 [0, 180] 범위로 조정
    if angle_diff_deg > 180:
        angle_diff_deg = 360 - angle_diff_deg

    # 그림 그리기
    plt.figure(figsize=(6,6))
    plt.plot(circ_x, circ_y, 'lightgray', label="Circle")
    plt.plot(arc_x, arc_y, 'r', linewidth=3, label="Shortest Arc")
    plt.scatter([0, x], [0, y], c='b', zorder=5)
    plt.scatter([cx], [cy], c='k', marker='x', label="Center")
    
    # 각도 정보 텍스트 추가
    plt.text(x, y + 1, f"End Angle: {angle_end_deg:.2f}°", fontsize=10, ha='center')
    plt.text(0, 0 + 1, f"Start Angle: {angle_start_deg:.2f}°", fontsize=10, ha='center')
    plt.text(0.5 * x, 0.5 * y - 2, f"Angle Diff: {angle_diff_deg:.2f}°", fontsize=12, ha='center', fontweight='bold', bbox=dict(facecolor='yellow', alpha=0.5))

    # 접선 시각화 (선택 사항)
    plt.plot([0, 0], [0, 5], 'g--', label="Start Tangent")
    plt.plot([x, x + 5*math.sin(angle_end_rad)], [y, y + 5*math.cos(angle_end_rad)], 'g--', label="End Tangent")

    plt.axhline(0, color='black', linewidth=1)
    plt.axvline(0, color='black', linewidth=1)
    plt.gca().set_aspect('equal', adjustable='box')
    plt.legend()
    plt.title(f"Circle: x² + (y-{r:.2f})² = {r:.2f}²")
    plt.show()

# 예시 실행
draw_arc_with_angles(30, )