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
#include <QPainter>

#include <qt_window.h>
#include <recidia.h>

using namespace std;

// Special widget for vertical text
VerticalLabel::VerticalLabel(const QString &text, QWidget *parent)
: QLabel(text, parent) {}

void VerticalLabel::paintEvent(QPaintEvent*) {
    QPainter painter(this);

    painter.translate(0,sizeHint().height());
    painter.rotate(270);
    painter.drawText(QRect (QPoint(0,0),QLabel::sizeHint()),Qt::AlignCenter,text());
}

QSize VerticalLabel::minimumSizeHint() const {
    QSize s = QLabel::minimumSizeHint();
    return QSize(s.height(), s.width());
}

QSize VerticalLabel::sizeHint() const {
    QSize s = QLabel::sizeHint();
    return QSize(s.height(), s.width());
}


static void set_button_color(QPushButton *button, QColor color) {

    QString textColor;
    if (color.black() > 150)
        textColor = "#FFFFFF";
    else
        textColor = "#000000";
    // "border: none;" for solid color
    button->setStyleSheet("color: " + textColor + ";"
                          "background: " + color.name() + ";");
}

SettingsTabWidget::SettingsTabWidget() {

    QWidget *dataTab = new QWidget(this);
    this->addTab(dataTab, "Data");

    QGridLayout *dataTabLayout = new QGridLayout;
    dataTab->setLayout(dataTabLayout);

    QLabel *heightCapLabel = new QLabel(QString::number(recidia_settings.data.height_cap), this);
    dataTabLayout->addWidget(heightCapLabel, 0, 0, 1, 2);
    VerticalLabel *heightCapTextLabel = new VerticalLabel("Height Cap", this);
    dataTabLayout->addWidget(heightCapTextLabel, 1, 0, 5, 1);

    heightCapSlider = new QSlider(Qt::Vertical, this);
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

            heightCapLabel->setText(QString::number((int) round(recidia_settings.data.height_cap)));
            // Allows for recursive checking
            QTimer::singleShot(10, heightCapSlider, &QSlider::sliderPressed);
        }
    });
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

            heightCapLabel->setText(QString::number((int) round(recidia_settings.data.height_cap)));
            heightCapSlider->setValue(0);
        }
    });
    QObject::connect(heightCapSlider, &QSlider::sliderReleased,
    [=]() {
        heightCapSlider->setValue(0);
    });
    dataTabLayout->addWidget(heightCapSlider, 1, 1, 5, 1);

    QLabel *savgolWindowSizeLabel = new QLabel("SavGol Window " + QString::number(recidia_settings.data.savgol_filter.window_size*100) +"%", this);
    dataTabLayout->addWidget(savgolWindowSizeLabel, 0, 2);
    savgolWindowSizeSlider = new QSlider(Qt::Horizontal, this);
    savgolWindowSizeSlider->setRange(0, 100);
    savgolWindowSizeSlider->setValue(recidia_settings.data.savgol_filter.window_size * 100);
    QObject::connect(savgolWindowSizeSlider, &QSlider::valueChanged,
    [=](int value) {
        recidia_settings.data.savgol_filter.window_size = (float) value / 100;
        savgolWindowSizeLabel->setText("SavGol Window " + QString::number(recidia_settings.data.savgol_filter.window_size*100) +"%");
    });
    dataTabLayout->addWidget(savgolWindowSizeSlider, 1, 2);

    QLabel *interpolationLabel = new QLabel("Interpolation " + QString::number(recidia_settings.data.interp) + "x", this);
    dataTabLayout->addWidget(interpolationLabel, 2, 2);
    interpolationSlider = new QSlider(Qt::Horizontal, this);
    interpolationSlider->setRange(0, recidia_settings.data.INTERP.MAX);
    interpolationSlider->setValue(recidia_settings.data.interp);
    QObject::connect(interpolationSlider, &QSlider::valueChanged,
    [=](int value) {
        recidia_settings.data.interp = value;
        interpolationLabel->setText("Interpolation " + QString::number(recidia_settings.data.interp) + "x");
    });
    dataTabLayout->addWidget(interpolationSlider, 3, 2);

    QLabel *audioBufferSizeLabel = new QLabel("Audio Buffer " + QString::number(recidia_settings.data.audio_buffer_size), this);
    dataTabLayout->addWidget(audioBufferSizeLabel, 4, 2);
    audioBufferSizeSlider = new QSlider(Qt::Horizontal, this);
    audioBufferSizeSlider->setRange(log2(1024), log2(recidia_settings.data.AUDIO_BUFFER_SIZE.MAX));
    audioBufferSizeSlider->setValue(log2(recidia_settings.data.audio_buffer_size));
    audioBufferSizeSlider->setPageStep(1);
    QObject::connect(audioBufferSizeSlider, &QSlider::valueChanged,
    [=](int value) {
        recidia_settings.data.audio_buffer_size = pow(2, value);
        audioBufferSizeLabel->setText("Audio Buffer " + QString::number(recidia_settings.data.audio_buffer_size));
    });
    dataTabLayout->addWidget(audioBufferSizeSlider, 5, 2);

    // Hack to keep it looking nice
    QVBoxLayout *columnTwoDataTabLayout = new QVBoxLayout;

    QLabel *pollRateLabel = new QLabel("Poll Rate", this);
    columnTwoDataTabLayout->addWidget(pollRateLabel);
    pollRateSpinBox = new QSpinBox(this);
    pollRateSpinBox->setRange(1, recidia_settings.data.POLL_RATE.MAX);
    pollRateSpinBox->setValue(recidia_settings.data.poll_rate);
    pollRateSpinBox->setSuffix("ms");
    QObject::connect(pollRateSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
    [=](int value) {
        recidia_settings.data.poll_rate = value;
    });
    columnTwoDataTabLayout->addWidget(pollRateSpinBox);

    QLabel *statsLabel = new QLabel("Stats", this);
    columnTwoDataTabLayout->addWidget(statsLabel);
    statsButton = new QPushButton(this);
    if (recidia_settings.data.stats == 0)
        statsButton->setText("Disabled");
    else if (recidia_settings.data.stats == 1)
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
    });
    columnTwoDataTabLayout->addWidget(statsButton);

    dataTabLayout->addLayout(columnTwoDataTabLayout, 0, 3, 5, 3);


    QWidget *designTab = new QWidget(this);
    this->addTab(designTab, "Design");
    QGridLayout *designTabLayout = new QGridLayout;
    designTab->setLayout(designTabLayout);

    QWidget *dimenGrid = new QWidget(this);
    QGridLayout *dimenGridLayout = new QGridLayout;
    dimenGrid->setLayout(dimenGridLayout);

    QLabel *drawGridLabel = new QLabel("Draw Dimensions", dimenGrid);
    drawGridLabel->setAlignment(Qt::AlignHCenter);
    dimenGridLayout->addWidget(drawGridLabel, 0, 0, 1, 3);

    QLabel *drawXposLabel = new QLabel("X Pos", dimenGrid);
    dimenGridLayout->addWidget(drawXposLabel, 1, 0);
    QDoubleSpinBox *drawXposSpinBox = new QDoubleSpinBox(dimenGrid);
    drawXposSpinBox->setDecimals(3);
    drawXposSpinBox->setRange(-1.0, 1.0);
    drawXposSpinBox->setValue(recidia_settings.design.draw_x);
    drawXposSpinBox->setSingleStep(0.01);
    QObject::connect(drawXposSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=](double value) {
        recidia_settings.design.draw_x = value;
    });
    dimenGridLayout->addWidget(drawXposSpinBox, 2, 0);

    QLabel *drawYposLabel = new QLabel("Y Pos", dimenGrid);
    dimenGridLayout->addWidget(drawYposLabel, 1, 1);
    QDoubleSpinBox *drawYposSpinBox = new QDoubleSpinBox(dimenGrid);
    drawYposSpinBox->setDecimals(3);
    drawYposSpinBox->setRange(-1.0, 1.0);
    drawYposSpinBox->setValue(recidia_settings.design.draw_y);
    drawYposSpinBox->setSingleStep(0.01);
    QObject::connect(drawYposSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=](double value) {
        recidia_settings.design.draw_y = value;
    });
    dimenGridLayout->addWidget(drawYposSpinBox, 2, 1);

    QLabel *drawWidthLabel = new QLabel("Width", dimenGrid);
    dimenGridLayout->addWidget(drawWidthLabel, 3, 0);
    QDoubleSpinBox *drawWidthSpinBox = new QDoubleSpinBox(dimenGrid);
    drawWidthSpinBox->setDecimals(3);
    drawWidthSpinBox->setRange(0.0, 1.0);
    drawWidthSpinBox->setValue(recidia_settings.design.draw_width);
    drawWidthSpinBox->setSingleStep(0.01);
    QObject::connect(drawWidthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=](double value) {
        recidia_settings.design.draw_width = value;
    });
    dimenGridLayout->addWidget(drawWidthSpinBox, 4, 0);

    QLabel *drawHeightLabel = new QLabel("Height", dimenGrid);
    dimenGridLayout->addWidget(drawHeightLabel, 3, 1);
    QDoubleSpinBox *drawHeightSpinBox = new QDoubleSpinBox(dimenGrid);
    drawHeightSpinBox->setDecimals(3);
    drawHeightSpinBox->setRange(0.0, 1.0);
    drawHeightSpinBox->setValue(recidia_settings.design.draw_height);
    drawHeightSpinBox->setSingleStep(0.01);
    QObject::connect(drawHeightSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=](double value) {
        recidia_settings.design.draw_height = value;
    });
    dimenGridLayout->addWidget(drawHeightSpinBox, 4, 1);

    designTabLayout->addWidget(dimenGrid, 0, 0, 5, 1);

    QLabel *plotWidthLabel = new QLabel("Plot Width " + QString::number(recidia_settings.design.plot_width), this);
    designTabLayout->addWidget(plotWidthLabel, 0, 2);
    plotWidthSlider = new QSlider(Qt::Horizontal, this);
    plotWidthSlider->setRange(1, recidia_settings.design.PLOT_WIDTH.MAX);
    plotWidthSlider->setValue(recidia_settings.design.plot_width);
    QObject::connect(plotWidthSlider, &QSlider::valueChanged,
    [=](int value) {
        recidia_settings.design.plot_width = value;
        plotWidthLabel->setText("Plot Width " + QString::number(value));
    });
    designTabLayout->addWidget(plotWidthSlider, 1, 2);

    QLabel *gapWidthLabel = new QLabel("Gap Width " + QString::number(recidia_settings.design.gap_width), this);
    designTabLayout->addWidget(gapWidthLabel, 2, 2);
    gapWidthSlider = new QSlider(Qt::Horizontal, this);
    gapWidthSlider->setRange(0, recidia_settings.design.GAP_WIDTH.MAX);
    gapWidthSlider->setValue(recidia_settings.design.gap_width);
    QObject::connect(gapWidthSlider, &QSlider::valueChanged,
    [=](int value) {
        recidia_settings.design.gap_width = value;
        gapWidthLabel->setText("Gap Width " + QString::number(value));
    });
    designTabLayout->addWidget(gapWidthSlider, 3, 2);

    QLabel *drawModeLabel = new QLabel("Draw Mode", this);
    designTabLayout->addWidget(drawModeLabel, 0, 3);
    drawModeButton = new QPushButton(this);
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
    });
    designTabLayout->addWidget(drawModeButton, 1, 3);

    QLabel *minHeightLabel = new QLabel("Min Height", this);
    designTabLayout->addWidget(minHeightLabel, 2, 3);
    QDoubleSpinBox *minHeightSpinBox = new QDoubleSpinBox(this);
    minHeightSpinBox->setDecimals(3);
    minHeightSpinBox->setRange(0.0, 1.0);
    minHeightSpinBox->setValue(recidia_settings.design.min_plot_height);
    minHeightSpinBox->setSingleStep(0.01);
    QObject::connect(minHeightSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
    [=](double value) {
        recidia_settings.design.min_plot_height = value;
    });
    designTabLayout->addWidget(minHeightSpinBox, 3, 3);

    QLabel *mainColorLabel = new QLabel("Main Color", this);
    designTabLayout->addWidget(mainColorLabel, 0, 4);
    QPushButton *mainColorButton = new QPushButton("Modify", this);
    rgba_color mainColor = recidia_settings.design.main_color;
    set_button_color(mainColorButton, QColor(mainColor.red, mainColor.green, mainColor.blue));

    QObject::connect(mainColorButton, &QPushButton::pressed,
    [=]() {
        if (main_color_dialog_up) // Only 1 color dialog at a time
            return;

        rgba_color cur_color = recidia_settings.design.main_color;
        QColor old_color = QColor(cur_color.red, cur_color.green, cur_color.blue, cur_color.alpha);

        QColorDialog *dialog = new QColorDialog(old_color, this);
        dialog->setWindowTitle("Main Color");
        dialog->setOption(QColorDialog::ShowAlphaChannel);
        dialog->show();
        main_color_dialog_up = true;
        QObject::connect(dialog, &QColorDialog::currentColorChanged,
        [=](QColor new_color){
            recidia_settings.design.main_color.red = new_color.red();
            recidia_settings.design.main_color.green = new_color.green();
            recidia_settings.design.main_color.blue = new_color.blue();
            recidia_settings.design.main_color.alpha = new_color.alpha();
        });
        QObject::connect(dialog, &QDialog::accepted,
        [=](){
            rgba_color mainColor = recidia_settings.design.main_color;
            set_button_color(mainColorButton, QColor(mainColor.red, mainColor.green, mainColor.blue));
        });
        QObject::connect(dialog, &QDialog::rejected,
        [=](){
            recidia_settings.design.main_color.red = old_color.red();
            recidia_settings.design.main_color.green = old_color.green();
            recidia_settings.design.main_color.blue = old_color.blue();
            recidia_settings.design.main_color.alpha = old_color.alpha();
        });
        QObject::connect(dialog, &QDialog::finished,
        [=](){
            main_color_dialog_up = false;
        });
    });
    designTabLayout->addWidget(mainColorButton, 1, 4);

    QLabel *backColorLabel = new QLabel("Back Color", this);
    designTabLayout->addWidget(backColorLabel, 2, 4);
    QPushButton *backColorButton = new QPushButton("Modify", this);
    rgba_color backColor = recidia_settings.design.back_color;
    set_button_color(backColorButton, QColor(backColor.red, backColor.green, backColor.blue));

    QObject::connect(backColorButton, &QPushButton::pressed,
     [=]() {
        if (back_color_dialog_up) // Only 1 color dialog at a time
            return;

         rgba_color cur_color = recidia_settings.design.back_color;
         QColor old_color = QColor(cur_color.red, cur_color.green, cur_color.blue, cur_color.alpha);

         QColorDialog *dialog = new QColorDialog(old_color, this);
         dialog->setWindowTitle("Back Color");
         dialog->setOption(QColorDialog::ShowAlphaChannel);
         dialog->show();
         back_color_dialog_up = true;
         QObject::connect(dialog, &QColorDialog::currentColorChanged,
         [=](QColor new_color){
             recidia_settings.design.back_color.red = new_color.red();
             recidia_settings.design.back_color.green = new_color.green();
             recidia_settings.design.back_color.blue = new_color.blue();
             recidia_settings.design.back_color.alpha = new_color.alpha();
         });
         QObject::connect(dialog, &QDialog::accepted,
         [=](){
             rgba_color backColor = recidia_settings.design.back_color;
             set_button_color(backColorButton, QColor(backColor.red, backColor.green, backColor.blue));
         });
         QObject::connect(dialog, &QDialog::rejected,
         [=](){
             recidia_settings.design.back_color.red = old_color.red();
             recidia_settings.design.back_color.green = old_color.green();
             recidia_settings.design.back_color.blue = old_color.blue();
             recidia_settings.design.back_color.alpha = old_color.alpha();
         });
         QObject::connect(dialog, &QDialog::finished,
         [=](){
             back_color_dialog_up = false;
         });
     });
    designTabLayout->addWidget(backColorButton, 3, 4);

    QLabel *fpsCapLabel = new QLabel("FPS Cap:", this);
    fpsCapLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    designTabLayout->addWidget(fpsCapLabel, 4, 3);
    fpsCapSpinBox = new QSpinBox(this);
    fpsCapSpinBox->setRange(1, recidia_settings.design.FPS_CAP.MAX);
    fpsCapSpinBox->setValue(recidia_settings.design.fps_cap);
    fpsCapSpinBox->setSuffix(" FPS");
    QObject::connect(fpsCapSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
    [=](int value) {
        recidia_settings.design.fps_cap = value;
    });
    designTabLayout->addWidget(fpsCapSpinBox, 4, 4);
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
