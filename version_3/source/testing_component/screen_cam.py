import cv2
import board
import digitalio
import busio
from adafruit_rgb_display import ili9341
from PIL import Image

# Configuration for ILI9341 TFT display
TFT_CS = board.D8       # Chip Select
TFT_DC = board.D24      # Data/Command
TFT_RST = board.D25     # Reset
TFT_MOSI = board.MOSI   # SPI MOSI
TFT_SCLK = board.SCK    # SPI Clock
TFT_LED = board.D18     # Backlight (optional)

# Set up SPI bus and display interface
spi = busio.SPI(TFT_SCLK, MOSI=TFT_MOSI)
cs_pin = digitalio.DigitalInOut(TFT_CS)
dc_pin = digitalio.DigitalInOut(TFT_DC)
reset_pin = digitalio.DigitalInOut(TFT_RST)
backlight = digitalio.DigitalInOut(TFT_LED)

backlight.direction = digitalio.Direction.OUTPUT
backlight.value = True  # Turn on the backlight

# Create the display object with optimized baudrate
display = ili9341.ILI9341(
    spi,
    cs=cs_pin,
    dc=dc_pin,
    rst=reset_pin,
    baudrate=99999999,  # 99.99 MHz
    width=240,
    height=320,
)

# Open the video file
video_path = "/home/admin/video.mp4"  # Replace with your video file path
# video_path = 0
cap = cv2.VideoCapture(video_path)

# Resize video frames to fit the display
target_width, target_height = display.width, display.height

# Enable OpenCV optimizations
cv2.setUseOptimized(True)

while cap.isOpened():
    ret, frame = cap.read()
    if not ret:
        print("End of video or error reading frame.")
        break

    # Skip every other frame to improve playback speed
    # frame_count += 1
    # if frame_count % 2 != 0:
    #     continue

    # Resize the frame to the display size only if needed
    if frame.shape[1] != target_width or frame.shape[0] != target_height:
        frame = cv2.resize(frame, (target_width, target_height))

    # Convert the frame from BGR (OpenCV format) to RGB (Pillow format)
    frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    # Convert the frame to a Pillow Image
    image = Image.fromarray(frame_rgb)

    # Display the image on the screen
    display.image(image)

# Release the video capture object
cap.release()
print("Video playback finished.")
