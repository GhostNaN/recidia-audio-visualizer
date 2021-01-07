#include <string>
#include <vector>
#include <unistd.h>

#include <QApplication>
#include <QWheelEvent>
#include <QVulkanInstance>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>

#include <qt_window.h>
#include <recidia.h>

using namespace std;

MainWindow::MainWindow(VulkanWindow *private_vulkan_window) {
    this->setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    settings_tabs = new SettingsTabWidget;
    vulkan_window = private_vulkan_window;

    QWidget *wrapper = QWidget::createWindowContainer(vulkan_window);
    // Forward children events to main window
    vulkan_window->installEventFilter(this);
    settings_tabs->installEventFilter(this);

    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(settings_tabs);
    mainLayout->addWidget(wrapper, 1);

    QWidget *container = new QWidget();
    container->setLayout(mainLayout);

    this->setCentralWidget(container);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    (void) obj;

    // children -> MainWindow
    if (event->type() == QEvent::KeyRelease) {
        QCoreApplication::sendEvent(this, event);
        return true;
    }
    return false;
}

//void MainWindow::showEvent(QShowEvent *event) {
//    (void) event;
//    this->centralWidget()->resize(1000, 501);
//}

void MainWindow::resizeEvent(QResizeEvent *event) {
    recidia_data.width = event->size().width();
    recidia_data.height = event->size().height();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Alt) {
        if (settings_tabs->isHidden())
            settings_tabs->show();
        else
            settings_tabs->hide();
    }
    else if (event->key() == Qt::Key_Shift) {
        if (!is_windowless) {
            this->setWindowFlag(Qt::FramelessWindowHint, true);
            this->show();
            is_windowless = true;
        }
        else {
            this->setWindowFlag(Qt::FramelessWindowHint, false);
            this->show();
            is_windowless = false;
        }
    }
    else {
        string keyString = event->text().toStdString();
        char key = keyString.c_str()[0];

        settings_tabs->change_setting(key);
    }
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
//    inst.setLayers(QByteArrayList() << "VK_LAYER_KHRONOS_validation");
    inst.create();

    VulkanWindow *vulkanWindow = new VulkanWindow;
    vulkanWindow->setVulkanInstance(&inst);
    QVector<VkFormat> formats = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB};
    vulkanWindow->setPreferredColorFormats(formats);

    MainWindow mainWindow(vulkanWindow);
    mainWindow.setWindowTitle("ReCidia");

    mainWindow.resize(1000, 500);
    mainWindow.show();

    vulkanWindow->main_window = &mainWindow;

    return app.exec();
}

int display_audio_devices(vector<string> devices, vector<uint> pulse_indexes, vector<uint> port_indexes) {
    const char *argv[] = {"", NULL};
    int argc = 1;
    QApplication app(argc, const_cast<char **>(argv));

    QDialog *dialog = new QDialog();

    QListWidget *pulseList = new QListWidget();
    QListWidget *portList = new QListWidget();

    QObject::connect(pulseList, &QListWidget::currentRowChanged,
    [=]() { portList->clearSelection(); } );

    QObject::connect(portList, &QListWidget::currentRowChanged,
    [=]() { pulseList->clearSelection(); } );

    QVBoxLayout *layout = new QVBoxLayout();

    uint i = 0;
    int d = 0;

    if (pulse_indexes.size()) {
        d = 1;
        QLabel *pulseLabel = new QLabel("PulseAudio Devices");
        pulseLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(pulseLabel);

        layout->addWidget(pulseList);

        pulseList->addItem("Default Output");
    }
    for (i=0; i < pulse_indexes.size(); i++) {
        pulseList->addItem(QString::fromStdString(devices[i]));
    }

    if (port_indexes.size()) {
        QLabel *portLabel = new QLabel("PortAudio Devices");
        portLabel->setAlignment(Qt::AlignHCenter);
        layout->addWidget(portLabel);

        layout->addWidget(portList);
    }
    for (uint j=0; j < port_indexes.size(); j++) {
        portList->addItem(QString::fromStdString(devices[i+j]));
    }

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *okButton = new QPushButton("Ok");
    QObject::connect(okButton, &QPushButton::released,
    [=]() { dialog->done(0); } );
    buttonLayout->addWidget(okButton);

    QPushButton *cancelButton = new QPushButton("Cancel");
    QObject::connect(cancelButton, &QPushButton::released,
    [=]() { exit(0); } );
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    dialog->setLayout(layout);
    dialog->resize(600, 450);
    dialog->exec();


    int index = 0;
    if (pulseList->selectedItems().size() != 0)
        index = pulseList->currentRow();
    else if (portList->selectedItems().size() != 0)
        index = portList->currentRow() + pulse_indexes.size()+d;

    return index;
}
