#include <string>
#include <cmath>

#include <QKeyEvent>
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

SettingsTabWidget::SettingsTabWidget() {

    QWidget *dataTab = new QWidget();
    this->addTab(dataTab, "Data");

    QGridLayout *dataTabLayout = new QGridLayout();
    dataTab->setLayout(dataTabLayout);

    QLabel *heightCapLabel = new QLabel("Height Cap\n" + QString::number(recidia_settings.data.height_cap));
    dataTabLayout->addWidget(heightCapLabel, 0, 0);
    heightCapSlider = new QSlider(Qt::Vertical);
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
    savgolWindowSizeSlider = new QSlider(Qt::Horizontal);
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
    interpolationSlider = new QSlider(Qt::Horizontal);
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
    audioBufferSizeSlider = new QSlider(Qt::Horizontal);
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
    pollRateSpinBox = new QSpinBox();
    pollRateSpinBox->setRange(1, recidia_settings.data.POLL_RATE.MAX);
    pollRateSpinBox->setValue(recidia_settings.data.poll_rate);
    pollRateSpinBox->setSuffix("ms");
    QObject::connect(pollRateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
    [=]() {
        recidia_settings.data.poll_rate = pollRateSpinBox->value();
    } );
    dataTabLayout->addWidget(pollRateSpinBox, 1, 2, 3, 2, Qt::AlignTop);

    QLabel *statsLabel = new QLabel("Stats");
    dataTabLayout->addWidget(statsLabel, 2, 2, 4, 2);
    statsButton = new QPushButton();
    if (recidia_settings.misc.stats == 0)
        statsButton->setText("Disabled");
    else if (recidia_settings.misc.stats == 1)
        statsButton->setText("Enabled");
    QObject::connect(statsButton, &QPushButton::pressed,
    [=]() {
        if (statsButton->text() == "Disabled") {
            main_window->stats_bar->show();
            statsButton->setText("Enabled");
        }
        else if (statsButton->text() == "Enabled") {
            main_window->stats_bar->hide();
            statsButton->setText("Disabled");
        }
    } );
    dataTabLayout->addWidget(statsButton, 3, 2, 5, 2, Qt::AlignBottom);


    QWidget *designTab = new QWidget();
    this->addTab(designTab, "Design");
    QGridLayout *designTabLayout = new QGridLayout();
    designTab->setLayout(designTabLayout);

    QWidget *dimenGrid = new QWidget();
    QGridLayout *dimenGridLayout = new QGridLayout();
    dimenGrid->setLayout(dimenGridLayout);

    QLabel *drawGridLabel = new QLabel("Draw Dimensions");
    drawGridLabel->setAlignment(Qt::AlignHCenter);
    dimenGridLayout->addWidget(drawGridLabel, 0, 0, 1, 3);

    QLabel *drawXposLabel = new QLabel("X Pos");
    dimenGridLayout->addWidget(drawXposLabel, 1, 0);
    QDoubleSpinBox *drawXposSpinBox = new QDoubleSpinBox();
    drawXposSpinBox->setDecimals(3);
    drawXposSpinBox->setRange(-1.0, 1.0);
    drawXposSpinBox->setValue(recidia_settings.design.draw_x);
    drawXposSpinBox->setSingleStep(0.01);
    QObject::connect(drawXposSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=]() {
        recidia_settings.design.draw_x = drawXposSpinBox->value();
    } );
    dimenGridLayout->addWidget(drawXposSpinBox, 2, 0);

    QLabel *drawYposLabel = new QLabel("Y Pos");
    dimenGridLayout->addWidget(drawYposLabel, 1, 1);
    QDoubleSpinBox *drawYposSpinBox = new QDoubleSpinBox();
    drawYposSpinBox->setDecimals(3);
    drawYposSpinBox->setRange(-1.0, 1.0);
    drawYposSpinBox->setValue(recidia_settings.design.draw_y);
    drawYposSpinBox->setSingleStep(0.01);
    QObject::connect(drawYposSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=]() {
        recidia_settings.design.draw_y = drawYposSpinBox->value();
    } );
    dimenGridLayout->addWidget(drawYposSpinBox, 2, 1);

    QLabel *drawWidthLabel = new QLabel("Width");
    dimenGridLayout->addWidget(drawWidthLabel, 3, 0);
    QDoubleSpinBox *drawWidthSpinBox = new QDoubleSpinBox();
    drawWidthSpinBox->setDecimals(3);
    drawWidthSpinBox->setRange(0.0, 1.0);
    drawWidthSpinBox->setValue(recidia_settings.design.draw_width);
    drawWidthSpinBox->setSingleStep(0.01);
    QObject::connect(drawWidthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=]() {
        recidia_settings.design.draw_width = drawWidthSpinBox->value();
    } );
    dimenGridLayout->addWidget(drawWidthSpinBox, 4, 0);

    QLabel *drawHeightLabel = new QLabel("Height");
    dimenGridLayout->addWidget(drawHeightLabel, 3, 1);
    QDoubleSpinBox *drawHeightSpinBox = new QDoubleSpinBox();
    drawHeightSpinBox->setDecimals(3);
    drawHeightSpinBox->setRange(0.0, 1.0);
    drawHeightSpinBox->setValue(recidia_settings.design.draw_height);
    drawHeightSpinBox->setSingleStep(0.01);
    QObject::connect(drawHeightSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=]() {
        recidia_settings.design.draw_height = drawHeightSpinBox->value();
    } );
    dimenGridLayout->addWidget(drawHeightSpinBox, 4, 1);

    designTabLayout->addWidget(dimenGrid, 0, 0, 5, 1);

    QLabel *plotWidthLabel = new QLabel("Plot Width " + QString::number(recidia_settings.design.plot_width));
    designTabLayout->addWidget(plotWidthLabel, 0, 1);
    plotWidthSlider = new QSlider(Qt::Horizontal);
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
    gapWidthSlider = new QSlider(Qt::Horizontal);
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
    drawModeButton = new QPushButton();
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
    fpsCapSpinBox = new QSpinBox();
    fpsCapSpinBox->setRange(1, recidia_settings.design.FPS_CAP.MAX);
    fpsCapSpinBox->setValue(recidia_settings.design.fps_cap);
    fpsCapSpinBox->setSuffix(" FPS");
    QObject::connect(fpsCapSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
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
}

// Overload to turn key to a change
void SettingsTabWidget::change_setting(char key) {
    int change = get_setting_change(key);
    SettingsTabWidget::change_setting(change);
}

void SettingsTabWidget::change_setting(int change) {

    switch (change) {
        case PLOT_HEIGHT_CAP_DECREASE:
            heightCapSlider->setValue(3);
            break;
        case PLOT_HEIGHT_CAP_INCREASE:
            heightCapSlider->setValue(-3);
            break;

        case SAVGOL_WINDOW_SIZE_DECREASE:
            savgolWindowSizeSlider->setValue(savgolWindowSizeSlider->value() - 1);
            break;
        case SAVGOL_WINDOW_SIZE_INCREASE:
            savgolWindowSizeSlider->setValue(savgolWindowSizeSlider->value() + 1);
            break;

        case INTERPOLATION_DECREASE:
            interpolationSlider->setValue(interpolationSlider->value() - 1);
            break;
        case INTERPOLATION_INCREASE:
            interpolationSlider->setValue(interpolationSlider->value() + 1);
            break;

        case AUDIO_BUFFER_SIZE_DECREASE:
            audioBufferSizeSlider->setValue(audioBufferSizeSlider->value() - 1);
            break;
        case AUDIO_BUFFER_SIZE_INCREASE:
            audioBufferSizeSlider->setValue(audioBufferSizeSlider->value() + 1);
            break;

        case POLL_RATE_DECREASE:
            pollRateSpinBox->setValue(pollRateSpinBox->value() - 1);
            pollRateSpinBox->editingFinished();
            break;
        case POLL_RATE_INCREASE:
            pollRateSpinBox->setValue(pollRateSpinBox->value() + 1);
            pollRateSpinBox->editingFinished();
            break;

        case STATS_TOGGLE:
            statsButton->pressed();
            break;


        case PLOT_WIDTH_DECREASE:
            plotWidthSlider->setValue(plotWidthSlider->value() - 1);
            break;
        case PLOT_WIDTH_INCREASE:
            plotWidthSlider->setValue(plotWidthSlider->value() + 1);
            break;

        case GAP_WIDTH_DECREASE:
            gapWidthSlider->setValue(gapWidthSlider->value() - 1);
            break;
        case GAP_WIDTH_INCREASE:
            gapWidthSlider->setValue(gapWidthSlider->value() + 1);
            break;

        case DRAW_MODE_TOGGLE:
            drawModeButton->pressed();
            break;

        case FPS_CAP_DECREASE:
            fpsCapSpinBox->setValue(fpsCapSpinBox->value() - 1);
            fpsCapSpinBox->editingFinished();
            break;
        case FPS_CAP_INCREASE:
            fpsCapSpinBox->setValue(fpsCapSpinBox->value() + 1);
            fpsCapSpinBox->editingFinished();
            break;
    }
}
