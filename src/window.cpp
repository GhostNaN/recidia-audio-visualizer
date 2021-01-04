#include <string>
#include <vector>
#include <cmath>
#include <unistd.h>

#include <QApplication>
#include <QWheelEvent>
#include <QVulkanInstance>
#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QGridLayout>
#include <QSlider>
#include <QColorDialog>
#include <QSpinBox>
#include <QTimer>

#include <qt_window.h>
#include <recidia.h>

using namespace std;

MainWindow::MainWindow(VulkanWindow *vulkan_window) {
    this->setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *mainLayout = new QVBoxLayout();

    settings_tabs = new QTabWidget();

    QWidget *dataTab = new QWidget();
    settings_tabs->addTab(dataTab, "Data");
    QGridLayout *dataTabLayout = new QGridLayout();
    dataTab->setLayout(dataTabLayout);

    QLabel *heightCapLabel = new QLabel("Height Cap\n" + QString::number(recidia_settings.data.height_cap));
    dataTabLayout->addWidget(heightCapLabel, 0, 0);
    QSlider *heightCapSlider = new QSlider(Qt::Vertical);
    heightCapSlider->setRange(-10, 10);
    heightCapSlider->setValue(0);
    // For mouse dragging only
    QObject::connect(heightCapSlider, &QSlider::sliderPressed,
    [=]() {
        if (heightCapSlider->isSliderDown()) {
            float multiplier = (float) heightCapSlider->value() / 100;

            if (multiplier < 0) {
                recidia_settings.data.height_cap *= 1 + -multiplier;
                if (recidia_settings.data.height_cap > recidia_settings.data.HEIGHT_CAP.MAX)
                    recidia_settings.data.height_cap = recidia_settings.data.HEIGHT_CAP.MAX;
            }
            else {
                recidia_settings.data.height_cap /= 1 + multiplier;
                if (recidia_settings.data.height_cap < 1)
                    recidia_settings.data.height_cap = 1;
            }

            heightCapLabel->setText("Height Cap\n" + QString::number((int) round(recidia_settings.data.height_cap)));
            // Allows for recursive checking
            QTimer::singleShot(10, heightCapSlider, &QSlider::sliderPressed);
        }
    } );
    // For the rest
    QObject::connect(heightCapSlider, &QSlider::valueChanged,
    [=]() {
        if (!heightCapSlider->isSliderDown()) {
            float multiplier = (float) heightCapSlider->value() / 10;

            if (multiplier < 0) {
                recidia_settings.data.height_cap *= 1 + -multiplier;
                if (recidia_settings.data.height_cap > recidia_settings.data.HEIGHT_CAP.MAX)
                    recidia_settings.data.height_cap = recidia_settings.data.HEIGHT_CAP.MAX;
            }
            else {
                recidia_settings.data.height_cap /= 1 + multiplier;
                if (recidia_settings.data.height_cap < 1)
                    recidia_settings.data.height_cap = 1;
            }

            heightCapLabel->setText("Height Cap\n" + QString::number((int) round(recidia_settings.data.height_cap)));
            heightCapSlider->setValue(0);
        }
    } );
    QObject::connect(heightCapSlider, &QSlider::sliderReleased,
    [=]() {
        heightCapSlider->setValue(0);
    } );

    dataTabLayout->addWidget(heightCapSlider, 1, 0, 5, 0);

    QLabel *savgolWindowSizeLabel = new QLabel("SavGol Window " + QString::number(recidia_settings.data.savgol_filter.window_size*100) +"%");
    dataTabLayout->addWidget(savgolWindowSizeLabel, 0, 1);
    QSlider *savgolWindowSizeSlider = new QSlider(Qt::Horizontal);
    savgolWindowSizeSlider->setRange(0, 100);
    savgolWindowSizeSlider->setValue(recidia_settings.data.savgol_filter.window_size);
    QObject::connect(savgolWindowSizeSlider, &QSlider::valueChanged,
    [=]() {
        recidia_settings.data.savgol_filter.window_size = (float) savgolWindowSizeSlider->value() / 100;
        savgolWindowSizeLabel->setText("SavGol Window " + QString::number(recidia_settings.data.savgol_filter.window_size*100) +"%");
    } );
    dataTabLayout->addWidget(savgolWindowSizeSlider, 1, 1);

    QLabel *interpolationLabel = new QLabel("Interpolation " + QString::number(recidia_settings.data.interp) + "x");
    dataTabLayout->addWidget(interpolationLabel, 2, 1);
    QSlider *interpolationSlider = new QSlider(Qt::Horizontal);
    interpolationSlider->setRange(0, recidia_settings.data.INTERP.MAX);
    interpolationSlider->setValue(recidia_settings.data.interp);
    QObject::connect(interpolationSlider, &QSlider::valueChanged,
    [=]() {
        recidia_settings.data.interp = interpolationSlider->value();
        interpolationLabel->setText("Interpolation " + QString::number(recidia_settings.data.interp) + "x");
    } );
    dataTabLayout->addWidget(interpolationSlider, 3, 1);

    QLabel *audioBufferSizeLabel = new QLabel("Audio Buffer " + QString::number(recidia_settings.data.audio_buffer_size));
    dataTabLayout->addWidget(audioBufferSizeLabel, 4, 1);
    QSlider *audioBufferSizeSlider = new QSlider(Qt::Horizontal);
    audioBufferSizeSlider->setRange(log2(1024), log2(recidia_settings.data.AUDIO_BUFFER_SIZE.MAX));
    audioBufferSizeSlider->setValue(log2(recidia_settings.data.audio_buffer_size));
    audioBufferSizeSlider->setPageStep(1);
    QObject::connect(audioBufferSizeSlider, &QSlider::valueChanged,
    [=]() {
        recidia_settings.data.audio_buffer_size = pow(2, audioBufferSizeSlider->value());
        audioBufferSizeLabel->setText("Audio Buffer " + QString::number(recidia_settings.data.audio_buffer_size));
    } );
    dataTabLayout->addWidget(audioBufferSizeSlider, 5, 1);

    QLabel *pollRateLabel = new QLabel("Poll Rate");
    dataTabLayout->addWidget(pollRateLabel, 0, 2, 1, 2);
    QSpinBox *pollRateSpinBox = new QSpinBox();
    pollRateSpinBox->setRange(1, recidia_settings.data.POLL_RATE.MAX);
    pollRateSpinBox->setValue(recidia_settings.data.poll_rate);
    pollRateSpinBox->setSuffix("ms");
    QObject::connect(pollRateSpinBox, &QSpinBox::editingFinished,
    [=]() {
        recidia_settings.data.poll_rate = pollRateSpinBox->value();
    } );
    dataTabLayout->addWidget(pollRateSpinBox, 1, 2, 2, 1, Qt::AlignTop);

    QLabel *hideSettingsLabel = new QLabel("Press \"Alt\"\n"
                                            "to hide");
    hideSettingsLabel->setStyleSheet("font-style: italic");
    dataTabLayout->addWidget(hideSettingsLabel, 4, 2, 5, 2);


    QWidget *designTab = new QWidget();
    settings_tabs->addTab(designTab, "Design");
    QGridLayout *designTabLayout = new QGridLayout();
    designTab->setLayout(designTabLayout);

    QWidget *dimenGrid = new QWidget();
    QGridLayout *dimenGridLayout = new QGridLayout();
    dimenGrid->setLayout(dimenGridLayout);

    QLabel *drawGridLabel = new QLabel("Draw Dimentions");
    drawGridLabel->setAlignment(Qt::AlignHCenter);
    dimenGridLayout->addWidget(drawGridLabel, 0, 0, 1, 3);

    QLabel *drawXposLabel = new QLabel("X Pos");
    dimenGridLayout->addWidget(drawXposLabel, 1, 0);
    QDoubleSpinBox *drawXposSpinBox = new QDoubleSpinBox();
    drawXposSpinBox->setRange(-1.0, 1.0);
    drawXposSpinBox->setValue(recidia_settings.design.draw_x);
    drawXposSpinBox->setSingleStep(0.01);
    QObject::connect(drawXposSpinBox, &QDoubleSpinBox::editingFinished,
    [=]() {
        recidia_settings.design.draw_x = drawXposSpinBox->value();
    } );
    dimenGridLayout->addWidget(drawXposSpinBox, 2, 0);

    QLabel *drawYposLabel = new QLabel("Y Pos");
    dimenGridLayout->addWidget(drawYposLabel, 1, 1);
    QDoubleSpinBox *drawYposSpinBox = new QDoubleSpinBox();
    drawYposSpinBox->setRange(-1.0, 1.0);
    drawYposSpinBox->setValue(recidia_settings.design.draw_y);
    drawYposSpinBox->setSingleStep(0.01);
    QObject::connect(drawYposSpinBox, &QDoubleSpinBox::editingFinished,
    [=]() {
        recidia_settings.design.draw_y = drawYposSpinBox->value();
    } );
    dimenGridLayout->addWidget(drawYposSpinBox, 2, 1);

    QLabel *drawWidthLabel = new QLabel("Width");
    dimenGridLayout->addWidget(drawWidthLabel, 3, 0);
    QDoubleSpinBox *drawWidthSpinBox = new QDoubleSpinBox();
    drawWidthSpinBox->setRange(0.0, 1.0);
    drawWidthSpinBox->setValue(recidia_settings.design.draw_width);
    drawWidthSpinBox->setSingleStep(0.01);
    QObject::connect(drawWidthSpinBox, &QDoubleSpinBox::editingFinished,
    [=]() {
        recidia_settings.design.draw_width = drawWidthSpinBox->value();
    } );
    dimenGridLayout->addWidget(drawWidthSpinBox, 4, 0);

    QLabel *drawHeightLabel = new QLabel("Height");
    dimenGridLayout->addWidget(drawHeightLabel, 3, 1);
    QDoubleSpinBox *drawHeightSpinBox = new QDoubleSpinBox();
    drawHeightSpinBox->setRange(0.0, 1.0);
    drawHeightSpinBox->setValue(recidia_settings.design.draw_height);
    drawHeightSpinBox->setSingleStep(0.01);
    QObject::connect(drawHeightSpinBox, &QDoubleSpinBox::editingFinished,
    [=]() {
        recidia_settings.design.draw_height = drawHeightSpinBox->value();
    } );
    dimenGridLayout->addWidget(drawHeightSpinBox, 4, 1);

    designTabLayout->addWidget(dimenGrid, 0, 0, 5, 1);

    QLabel *plotWidthLabel = new QLabel("Plot Width " + QString::number(recidia_settings.design.plot_width));
    designTabLayout->addWidget(plotWidthLabel, 0, 1);
    QSlider *plotWidthSlider = new QSlider(Qt::Horizontal);
    plotWidthSlider->setRange(1, recidia_settings.design.PLOT_WIDTH.MAX);
    plotWidthSlider->setValue(recidia_settings.design.plot_width);
    QObject::connect(plotWidthSlider, &QSlider::valueChanged,
    [=]() {
        recidia_settings.design.plot_width = plotWidthSlider->value();
        plotWidthLabel->setText("Plot Width " + QString::number(recidia_settings.design.plot_width));
    } );
    designTabLayout->addWidget(plotWidthSlider, 1, 1);

    QLabel *gapWidthLabel = new QLabel("Gap Width " + QString::number(recidia_settings.design.gap_width));
    designTabLayout->addWidget(gapWidthLabel, 2, 1);
    QSlider *gapWidthSlider = new QSlider(Qt::Horizontal);
    gapWidthSlider->setRange(0, recidia_settings.design.GAP_WIDTH.MAX);
    gapWidthSlider->setValue(recidia_settings.design.gap_width);
    QObject::connect(gapWidthSlider, &QSlider::valueChanged,
    [=]() {
        recidia_settings.design.gap_width = gapWidthSlider->value();
        gapWidthLabel->setText("Gap Width " + QString::number(recidia_settings.design.gap_width));
    } );
    designTabLayout->addWidget(gapWidthSlider, 3, 1);

    QLabel *drawModeLabel = new QLabel("Draw Mode");
    designTabLayout->addWidget(drawModeLabel, 0, 2);
    QPushButton *drawModeButton = new QPushButton();
    if (recidia_settings.design.draw_mode == 0)
        drawModeButton->setText("Bars");
    else if (recidia_settings.design.draw_mode == 1)
        drawModeButton->setText("Points");
    QObject::connect(drawModeButton, &QPushButton::pressed,
    [=]() {
        if (drawModeButton->text() == "Bars") {
            recidia_settings.design.draw_mode = 1;
            drawModeButton->setText("Points");
        }
        else if (drawModeButton->text() == "Points") {
            recidia_settings.design.draw_mode = 0;
            drawModeButton->setText("Bars");
        }
    } );
    designTabLayout->addWidget(drawModeButton, 1, 2);

    QLabel *fpsCapLabel = new QLabel("FPS Cap");
    designTabLayout->addWidget(fpsCapLabel, 2, 2);
    QSpinBox *fpsCapSpinBox = new QSpinBox();
    fpsCapSpinBox->setRange(1, recidia_settings.design.FPS_CAP.MAX);
    fpsCapSpinBox->setValue(recidia_settings.design.fps_cap);
    fpsCapSpinBox->setSuffix(" FPS");
    QObject::connect(fpsCapSpinBox, &QSpinBox::editingFinished,
    [=]() {
        recidia_settings.design.fps_cap = fpsCapSpinBox->value();
    } );
    designTabLayout->addWidget(fpsCapSpinBox, 3, 2);

    rgba_color color = recidia_settings.design.main_color;
    QPixmap pixmap(100,100);

    QLabel *mainColorLabel = new QLabel("Main Color");
    designTabLayout->addWidget(mainColorLabel, 0, 3);
    pixmap.fill(QColor(color.red, color.green, color.blue));
    QPushButton *mainColorButton = new QPushButton(QIcon(pixmap), "");
    QObject::connect(mainColorButton, &QPushButton::pressed,
    [=]() {
        rgba_color cur_color = recidia_settings.design.main_color;
        QColor old_color = QColor(cur_color.red, cur_color.green, cur_color.blue, cur_color.alpha);

        QColor new_color = QColorDialog::getColor(old_color, this, "Main Color", QColorDialog::ShowAlphaChannel);
        recidia_settings.design.main_color.red = new_color.red();
        recidia_settings.design.main_color.green = new_color.green();
        recidia_settings.design.main_color.blue = new_color.blue();
        recidia_settings.design.main_color.alpha = new_color.alpha();

        QPixmap pixmap(100,100);
        pixmap.fill(QColor(new_color.red(), new_color.green(), new_color.blue()));
        mainColorButton->setIcon(QIcon(pixmap));
    } );
    designTabLayout->addWidget(mainColorButton, 1, 3);

    color = recidia_settings.design.back_color;

    QLabel *backColorLabel = new QLabel("Back Color");
    designTabLayout->addWidget(backColorLabel, 2, 3);
    pixmap.fill(QColor(color.red, color.green, color.blue));
    QPushButton *backColorButton = new QPushButton(QIcon(pixmap), "");
    QObject::connect(backColorButton, &QPushButton::pressed,
    [=]() {
        rgba_color cur_color = recidia_settings.design.back_color;
        QColor old_color = QColor(cur_color.red, cur_color.green, cur_color.blue, cur_color.alpha);

        QColor new_color = QColorDialog::getColor(old_color, this, "Back Color", QColorDialog::ShowAlphaChannel);
        recidia_settings.design.back_color.red = new_color.red();
        recidia_settings.design.back_color.green = new_color.green();
        recidia_settings.design.back_color.blue = new_color.blue();
        recidia_settings.design.back_color.alpha = new_color.alpha();

        QPixmap pixmap(100,100);
        pixmap.fill(QColor(new_color.red(), new_color.green(), new_color.blue()));
        backColorButton->setIcon(QIcon(pixmap));
    } );
    designTabLayout->addWidget(backColorButton, 3, 3);


//    QWidget *miscTab = new QWidget();
//    settings_tabs->addTab(miscTab, "Misc");
//    QGridLayout *miscTabLayout = new QGridLayout();
//    miscTab->setLayout(miscTabLayout);

//    QSpinBox *fpsSpinBox = new QSpinBox();
//    fpsSpinBox->setRange(1, recidia_settings.design.fps_cap.MAX);
//    fpsSpinBox->setValue(recidia_settings.design.fps_cap);
//    fpsSpinBox->setSuffix(" FPS");
//    QObject::connect(fpsSpinBox, &QSpinBox::editingFinished,
//    [=]() {
//        recidia_settings.design.fps_cap = fpsSpinBox->value();
//    } );
//    miscTabLayout->addWidget(fpsSpinBox, 5, 1);


    QWidget *wrapper = QWidget::createWindowContainer(vulkan_window);
    // Forward vulkan_window events to main window
    vulkan_window->installEventFilter(this);

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

    if (event->type() == QEvent::KeyRelease) {
        QCoreApplication::sendEvent(this, event);
        return true;
    }
    return false;
}

void MainWindow::showEvent(QShowEvent *event) {
    (void) event;
    this->centralWidget()->resize(1000, 501);
}

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
        change_setting_by_key(key);
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    (void) event;
    exit(EXIT_SUCCESS);
}

// Only vulkan window changes height
void VulkanWindow::wheelEvent(QWheelEvent *event) {
    int mouseDir = event->angleDelta().y();

    if (mouseDir > 0) {
        if (recidia_settings.data.height_cap > 1) {
            recidia_settings.data.height_cap /= 1.25;

            if (recidia_settings.data.height_cap < 1)
                recidia_settings.data.height_cap = 1;
        }
    }
    else {
        if (recidia_settings.data.height_cap < recidia_settings.data.HEIGHT_CAP.MAX) {
            recidia_settings.data.height_cap *= 1.25;

            if (recidia_settings.data.height_cap > recidia_settings.data.HEIGHT_CAP.MAX)
                recidia_settings.data.height_cap = recidia_settings.data.HEIGHT_CAP.MAX;
        }
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
