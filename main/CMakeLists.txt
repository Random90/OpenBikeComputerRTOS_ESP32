idf_component_register(
    SRCS "main.c" "Tasks/screen_pcd8544/screen_pcd8544.c" "Tasks/screen_pcd8544/pcd8544_font_utils.c" "utils/math.c"
    INCLUDE_DIRS "Tasks" "Tasks/screen_pcd8544" "utils"
)
