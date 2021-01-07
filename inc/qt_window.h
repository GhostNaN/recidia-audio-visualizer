#include <QWidget>
#include <QMainWindow>
#include <QVulkanWindow>

#include <QPushButton>
#include <QSlider>
#include <QSpinBox>

#include <string>
#include <vector>

#include <recidia.h>

#pragma once

int display_audio_devices(std::vector<std::string> devices, std::vector<uint> pulse_indexes, std::vector<uint> port_indexes);

int init_gui(int argc, char *argv[]);

class MainWindow;


class SettingsTabWidget : public QTabWidget {
    
public:
    explicit SettingsTabWidget();
    // Turns keys to a change
    void change_setting(char key);
    void change_setting(int change);
    
private:
    QSlider *heightCapSlider;
    QSlider *savgolWindowSizeSlider;
    QSlider *interpolationSlider;
    QSlider *audioBufferSizeSlider;
    QSpinBox *pollRateSpinBox;
    
    QSlider *plotWidthSlider;
    QSlider *gapWidthSlider;
    QPushButton *drawModeButton;
    QSpinBox *fpsCapSpinBox;
};


class VulkanWindow : public QVulkanWindow {
    
public:
    QVulkanWindowRenderer *createRenderer() override;
    MainWindow *main_window;
    
protected:
    void wheelEvent(QWheelEvent *event) override;
};

class VulkanRenderer : public QVulkanWindowRenderer {
    
public:
    VulkanRenderer(VulkanWindow *w);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

private:
    double last_frame_time;
    
    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_buf = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    QMatrix4x4 m_proj;
    float m_rotation = 0.0f;
};


class MainWindow : public QMainWindow {
    
public:
    explicit MainWindow(VulkanWindow *w);
    
    VulkanWindow *vulkan_window;
    SettingsTabWidget *settings_tabs;
    
private:
    bool is_windowless = 0;
    
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    
    //void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
};
