# Untitled - By: Chen An - Mon May 26 2025

import sensor, image, time, math
from pyb import Servo, UART

# ---------- User Configuration ----------
target_x = 50  # Target center X coordinate
target_y = 52  # Target center Y coordinate
tolerance = 0  # Tolerance is 0
# ----------------------------------------

# Color thresholds
red_threshold = (86, 27, -122, 117, 61, -64)
black_threshold = (0, 20, -20, 10, -11, 31)

# Camera initialization
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QQVGA)
sensor.skip_frames(time=2000)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)
clock = time.clock()

# Servo initialization
s1 = Servo(1)
s1.angle(-35)

# UART initialization UART3, baud rate 9600
uart = UART(3, 9600)

# Status flags
red_found = False
grip_triggered = False
last_uart_time = time.ticks_ms()

while True:
    clock.tick()
    img = sensor.snapshot().lens_corr(1.8)
    img.draw_cross(target_x, target_y, color=(255, 0, 0))  # Draw the target point

    if not red_found:
        blobs = img.find_blobs([red_threshold], pixels_threshold=3000, area_threshold=3000, merge=True)
        for blob in blobs:
            if blob.elongation() < 0.5:
                img.draw_rectangle(blob.rect(), color=(255, 0, 0))
                red_found = True
                red_rect = blob.rect()
                print("Red rectangle detected.")
                break

    else:
        img.draw_rectangle(red_rect, color=(255, 0, 0))

        blobs = img.find_blobs([black_threshold], pixels_threshold=200, area_threshold=200, merge=True)
        for blob in blobs:
            cx = blob.cx()
            cy = blob.cy()

            img.draw_rectangle(blob.rect(), color=(0, 255, 0))
            img.draw_cross(cx, cy, color=(255, 255, 255))

            dx = target_x - cx
            dy = target_y - cy

            # print("Black Object Center: (x: {}, y: {})".format(cx, cy))
            # print("Distance to target: DX={}, DY={}".format(dx, dy))

            # Send the coordinate difference every 2 seconds
            current_time = time.ticks_ms()
            if time.ticks_diff(current_time, last_uart_time) >= 2000:
                # Limit the data range to 1 byte (-127~127)
                def clamp(val):
                    return max(-127, min(127, val))

                dx = clamp(dx)
                dy = clamp(dy)

                # Send information frame start flag
                uart.writechar(0xEE)

                # X direction sign
                if dx < 0:
                    uart.writechar(0x01)
                else:
                    uart.writechar(0x00)
                uart.writechar(abs(dx))

                # Y direction sign
                if dy < 0:
                    uart.writechar(0x01)
                else:
                    uart.writechar(0x00)
                uart.writechar(abs(dy))

                # Send frame end flag
                uart.writechar(0xFF)

                print("Sending data -> Start frame: 0xEE DX:{} {}, DY:{} {}, End frame: 0xFF".format(
                    '-' if dx < 0 else '+', abs(dx),
                    '-' if dy < 0 else '+', abs(dy)
                ))

                last_uart_time = current_time

            # If the coordinate difference is 0, grip the object
            if not grip_triggered and abs(dx) <= tolerance and abs(dy) <= tolerance:
                print("Target aligned. Gripping...")
                time.sleep(5)
                s1.angle(0)
                time.sleep(600)
                grip_triggered = True
            break
