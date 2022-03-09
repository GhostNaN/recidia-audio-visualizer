#include <string>
#include <vector>
#include <unistd.h>

#include <QApplication>
#include <QWheelEvent>
#include <QVulkanInstance>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>

#include <qt_window.hpp>
#include <recidia.h>

using namespace std;


MainWindow::MainWindow(VulkanWindow *private_vulkan_window) {
    this->setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    settings_tabs = new SettingsTabWidget;
    vulkan_window = private_vulkan_window;
    stats_bar = new StatsWidget;

    settings_tabs->main_window = this;
    vulkan_window->main_window = this;

    QWidget *wrapper = QWidget::createWindowContainer(vulkan_window);
    // Forward children events to main window
    vulkan_window->installEventFilter(this);
    settings_tabs->installEventFilter(this);

    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(settings_tabs);
    mainLayout->addWidget(wrapper, 1);
    mainLayout->addWidget(stats_bar);

    QWidget *container = new QWidget(this);
    container->setLayout(mainLayout);

    this->setCentralWidget(container);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    (void) obj;

    // children -> MainWindow
    if (event->type() == QEvent::KeyPress) {
        QCoreApplication::sendEvent(this, event);
        return true;
    }
    return false;
}

void MainWindow::keyPressEvent(QKeyEvent *event) {

    string keyString = event->text().toStdString();
    char key = keyString.c_str()[0];
    settings_tabs->change_setting(key);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    (void) event;
    exit(EXIT_SUCCESS);
}

void VulkanWindow::wheelEvent(QWheelEvent *event) {
    int mouseDir = event->angleDelta().y();

    if (mouseDir > 0) {
        main_window->settings_tabs->change_setting(PLOT_HEIGHT_CAP_DECREASE);
    }
    else {
        main_window->settings_tabs->change_setting(PLOT_HEIGHT_CAP_INCREASE);
    }
}

int init_gui(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QVulkanInstance inst;
    // inst.setLayers(QByteArrayList() << "VK_LAYER_KHRONOS_validation");
    inst.create();

    VulkanWindow *vulkanWindow = new VulkanWindow;
    vulkanWindow->setVulkanInstance(&inst);
    QVector<VkFormat> formats = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB};
    vulkanWindow->setPreferredColorFormats(formats);

    MainWindow mainWindow(vulkanWindow);
    mainWindow.setWindowTitle("ReCidia");
    mainWindow.resize(1000, 500);

    if (recidia_settings.misc.frameless)
        mainWindow.setWindowFlag(Qt::FramelessWindowHint, true);
    mainWindow.show();

    return app.exec();
}
