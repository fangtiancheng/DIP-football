/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

// author: FTC and LXD

#include "player/player.h"

#include "playercontrols.h"
#include "playlistmodel.h"
#include "histogramwidget.h"
#include "videowidget.h"

#include "player/qmediaplaylist.h"
#include <QMediaMetaData>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QMediaFormat>
#include <QtWidgets>

Player::Player(QWidget *parent, ConfigMessage* cfgmsg,
               Connection* cnnct, FileUploader* clnt)
    : QWidget(parent), configMessage(cfgmsg),
      connection(cnnct), fileUploader(clnt),
      calibrateProgress(new QProcess(this))
{
    qDebug() << configMessage->toJson();
//! [create-objs]
    m_player_left = new QMediaPlayer(this);
    m_player_right = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    // only left video has audio output
    m_player_left->setAudioOutput(m_audioOutput);
//! [create-objs]

//! [2]
    m_videoWidget_left = new VideoWidget(this);
    m_videoWidget_left->resize(1280, 720);
    m_player_left->setVideoOutput(m_videoWidget_left);
    m_videoWidget_left->setMinimumHeight(600);
    m_videoWidget_right = new VideoWidget(this);
    m_videoWidget_right->resize(1280, 720);
    m_player_right->setVideoOutput(m_videoWidget_right);
    m_videoWidget_right->setMinimumHeight(600);

//! [2]

    connect(m_player_left, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(m_player_left, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(m_player_left, &QMediaPlayer::mediaStatusChanged, this, &Player::statusChanged);
    connect(m_player_left, &QMediaPlayer::bufferProgressChanged, this, &Player::bufferingProgress);
    connect(m_player_left, &QMediaPlayer::hasVideoChanged, this, &Player::videoAvailableChanged);
    connect(m_player_left, &QMediaPlayer::errorChanged, this, &Player::displayErrorMessage);


    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setRange(0, m_player_left->duration());

    m_labelDuration = new QLabel(this);
    m_labelDuration->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(m_slider, &QSlider::sliderMoved, this, &Player::seek);

    QGridLayout *browserLayout = new QGridLayout;
    this->textBrowser = new QTextBrowser(this);
    browserLayout->addWidget(textBrowser, 0, 0, 3, 3);

    calibrationButton = new QPushButton(this);
    calibrationButton->setText("calibrate");
    loadJsonButton = new QPushButton(this);
    loadJsonButton->setText("load json");
    saveJsonButton = new QPushButton(this);
    saveJsonButton->setText("save json");
    saveJsonButton->setEnabled(false);
    connect(saveJsonButton, &QAbstractButton::clicked, this, &Player::saveJson);
    QPushButton *openButton = new QPushButton(tr("Open"), this);
    connect(openButton, &QPushButton::clicked, this, &Player::open);

    PlayerControls *controls = new PlayerControls(this);
    controls->setState(m_player_left->playbackState());
    controls->setVolume(m_audioOutput->volume());
    controls->setMuted(controls->isMuted());

    playerDisplayModel = new QSortFilterProxyModel(this);
    playerDisplayView = new QTreeView(this);
    playerDisplayView->setRootIsDecorated(false);
    playerDisplayView->setAlternatingRowColors(true);
    playerDisplayView->setModel(playerDisplayModel);
    playerDisplayView->setSortingEnabled(true);
    browserLayout->addWidget(playerDisplayView, 0, 3, 3, 5);

// ****************************************************************************************************
    connect(calibrationButton, &QPushButton::clicked, this, &Player::calibrate);
    connect(loadJsonButton, &QPushButton::clicked, this, &Player::loadPlayerModel);
    connect(controls, &PlayerControls::play, m_player_left, &QMediaPlayer::play);
    connect(controls, &PlayerControls::pause, m_player_left, &QMediaPlayer::pause);
    connect(controls, &PlayerControls::stop, m_player_left, &QMediaPlayer::stop);
    connect(controls, &PlayerControls::pause, this, &Player::loadPlayerModel);
    connect(controls, &PlayerControls::next, this, &Player::changeSource);
    connect(controls, &PlayerControls::previous, this, &Player::previousClicked);
    connect(controls, &PlayerControls::changeVolume, m_audioOutput, &QAudioOutput::setVolume);
    connect(controls, &PlayerControls::changeMuting, m_audioOutput, &QAudioOutput::setMuted);
    connect(controls, &PlayerControls::changeRate, m_player_left, &QMediaPlayer::setPlaybackRate);
    connect(controls, &PlayerControls::stop, m_videoWidget_left, QOverload<>::of(&QVideoWidget::update));

    connect(m_player_left, &QMediaPlayer::playbackStateChanged, controls, &PlayerControls::setState);
    connect(m_audioOutput, &QAudioOutput::volumeChanged, controls, &PlayerControls::setVolume);
    connect(m_audioOutput, &QAudioOutput::mutedChanged, controls, &PlayerControls::setMuted);

// ****************************************************************************************************
    connect(controls, &PlayerControls::play, m_player_right, &QMediaPlayer::play);
    connect(controls, &PlayerControls::pause, m_player_right, &QMediaPlayer::pause);
    connect(controls, &PlayerControls::stop, m_player_right, &QMediaPlayer::stop);
    connect(controls, &PlayerControls::changeRate, m_player_right, &QMediaPlayer::setPlaybackRate);
    connect(controls, &PlayerControls::stop, m_videoWidget_right, QOverload<>::of(&QVideoWidget::update));
    connect(m_player_right, &QMediaPlayer::playbackStateChanged, controls, &PlayerControls::setState);

// ****************************************************************************************************

//    m_fullScreenButton = new QPushButton(tr("FullScreen"), this);
//    m_fullScreenButton->setCheckable(true);


#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    m_audioOutputCombo = new QComboBox(this);
    m_audioOutputCombo->addItem(QString::fromUtf8("Default"), QVariant::fromValue(QAudioDevice()));
    for (auto &deviceInfo: QMediaDevices::audioOutputs())
        m_audioOutputCombo->addItem(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    connect(m_audioOutputCombo, QOverload<int>::of(&QComboBox::activated), this, &Player::audioOutputChanged);
#endif

    QBoxLayout *displayLayout = new QHBoxLayout;
    displayLayout->addWidget(m_videoWidget_left, 2);
    displayLayout->addWidget(m_videoWidget_right, 2);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)

#endif

    QBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->addWidget(openButton);
    controlLayout->addStretch(1);
    controlLayout->addWidget(controls);
    controlLayout->addStretch(1);
    controlLayout->addWidget(calibrationButton);
    controlLayout->addWidget(loadJsonButton);
    controlLayout->addWidget(saveJsonButton);
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    controlLayout->addWidget(m_audioOutputCombo);
#endif

    QBoxLayout *layout = new QVBoxLayout;
    layout->addLayout(displayLayout);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(m_slider);
    hLayout->addWidget(m_labelDuration);
    layout->addLayout(hLayout);
    layout->addLayout(controlLayout);
    layout->addLayout(browserLayout);
    setLayout(layout);

    if (!isPlayerAvailable()) {
        QMessageBox::warning(this, tr("Service not available"),
                             tr("The QMediaPlayer object does not have a valid service.\n"\
                                "Please check the media service plugins are installed."));

        controls->setEnabled(false);
        openButton->setEnabled(false);
    }

    setSource();
    connect(calibrateProgress, &QProcess::finished,
            this, &Player::calibrateFinished);
}

Player::~Player()
{
    delete frames1;
    delete frames2;
}

bool Player::isPlayerAvailable() const
{
    return m_player_left->isAvailable() && m_player_right->isAvailable();
}

void Player::open()
{
    qDebug("Click Open!\n");
    QString dirName = QFileDialog::getExistingDirectory(this, "Choose a work directory",
        "/home/ftc/football/data");
    if(!dirName.isEmpty()){
        configMessage->dirName = dirName;
        configMessage->load();
        setSource();
        this->textBrowser->append(configMessage->toJson());
    }
}


void Player::setSource(){
    QUrl mediaUrlLeft = QUrl::fromLocalFile(configMessage->videoPath);
    QUrl mediaUrlRight = QUrl::fromLocalFile(configMessage->result1Path);
    this->currentSource = 1;
//    qDebug() << "Set left Source from '"<< mediaUrlLeft<< "'\n";
//    qDebug() << "Set right Source from '"<< mediaUrlRight<< "'\n";
    m_player_left->setSource(mediaUrlLeft);
    m_player_right->setSource(mediaUrlRight);
}
void Player::setPlayerModel(QAbstractItemModel* model){
    this->playerDisplayModel->setSourceModel(model);
}
void Player::changeSource(){
    m_player_left->pause();
    m_player_right->pause();
    qint64 currentPosition = m_player_left->position();
    if(currentSource == 1){
        QUrl mediaUrlRight = QUrl::fromLocalFile(configMessage->result2Path);
        currentSource = 2;
        m_player_right->setSource(mediaUrlRight);
        textBrowser->append(tr("[%1] change track from 1 to 2").arg(getTimeInfo(currentPosition)));
    }
    else{
        // current Source == 2
        QUrl mediaUrlRight = QUrl::fromLocalFile(configMessage->result1Path);
        currentSource = 1;
        m_player_right->setSource(mediaUrlRight);
        textBrowser->append(tr("[%1] change track from 2 to 1").arg(getTimeInfo(currentPosition)));
    }
    m_player_right->setPosition(currentPosition);
}
bool Player::loadJson(){
    bool frame1New = false;
    bool frame2New = false;
    if(frames1 == nullptr){
        frame1New = true;
        frames1 = new QMap<int, QMap<int, FootballPlayer>>;
    }
    if(frames2 == nullptr){
        frame2New = true;
        frames2 = new QMap<int, QMap<int, FootballPlayer>>;
    }
    try{
        loadFrameJson(this->configMessage->json1Path, this->frames1);
        loadFrameJson(this->configMessage->json2Path, this->frames2);
    }
    catch(std::runtime_error e){
        textBrowser->append(tr("Load Json Error: ") + tr(e.what()));
        if(frame1New){
            delete frames1;
            frames1 = nullptr;
        }
        if(frame2New){
            delete frames2;
            frames2 = nullptr;
        }
        return false;
    }
    textBrowser->append(tr("Load Json From %1 and %2")
        .arg(configMessage->json1Path, configMessage->json2Path));
    saveJsonButton->setEnabled(true);
    return true;
}
void Player::updateSource(){
    this->textBrowser->append(this->configMessage->toJson());
    setSource();
}
void Player::durationChanged(qint64 duration)
{
    m_duration = duration;
    m_slider->setMaximum(m_duration);
}

void Player::positionChanged(qint64 progress)
{
    if (!m_slider->isSliderDown())
        m_slider->setValue(progress);

    updateDurationInfo(progress);
}


QString Player::trackName(const QMediaMetaData &metaData, int index)
{
    QString name;
    QString title = metaData.stringValue(QMediaMetaData::Title);
    QLocale::Language lang = metaData.value(QMediaMetaData::Language).value<QLocale::Language>();

    if (title.isEmpty()) {
        if (lang == QLocale::Language::AnyLanguage)
            name = tr("Track %1").arg(index+1);
        else
            name = QLocale::languageToString(lang);
    } else {
        if (lang == QLocale::Language::AnyLanguage)
            name = title;
        else
            name = QString("%1 - [%2]").arg(title).arg(QLocale::languageToString(lang));
    }
    return name;
}


void Player::previousClicked()
{
    m_player_left->setPosition(0);
    m_player_right->setPosition(0);
}


void Player::seek(int seconds)
{
    m_player_left->setPosition(seconds);
    m_player_right->setPosition(seconds);
}

void Player::statusChanged(QMediaPlayer::MediaStatus status)
{
    qDebug("in void Player::statusChanged\n");
    handleCursor(status);

    // handle status message
    switch (status) {
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
        setStatusInfo(QString());
        break;
    case QMediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        setStatusInfo(tr("Buffering %1%").arg(qRound(m_player_left->bufferProgress()*100.)));
        break;
    case QMediaPlayer::StalledMedia:
        setStatusInfo(tr("Stalled %1%").arg(qRound(m_player_left->bufferProgress()*100.)));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        break;
    case QMediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}


void Player::handleCursor(QMediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == QMediaPlayer::LoadingMedia ||
        status == QMediaPlayer::BufferingMedia ||
        status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
#endif
}

void Player::bufferingProgress(float progress)
{
    if (m_player_left->mediaStatus() == QMediaPlayer::StalledMedia)
        setStatusInfo(tr("Stalled %1%").arg(qRound(progress*100.)));
    else
        setStatusInfo(tr("Buffering %1%").arg(qRound(progress*100.)));
}

void Player::videoAvailableChanged(bool available)
{
    if (!available) {
//        disconnect(m_fullScreenButton, &QPushButton::clicked, m_videoWidget_left, &QVideoWidget::setFullScreen);
//        disconnect(m_videoWidget_left, &QVideoWidget::fullScreenChanged, m_fullScreenButton, &QPushButton::setChecked);
        m_videoWidget_left->setFullScreen(false);
        m_videoWidget_right->setFullScreen(false);
    } else {
//        connect(m_fullScreenButton, &QPushButton::clicked, m_videoWidget_left, &QVideoWidget::setFullScreen);
//        connect(m_videoWidget_left, &QVideoWidget::fullScreenChanged, m_fullScreenButton, &QPushButton::setChecked);

//        if (m_fullScreenButton->isChecked())
//            m_videoWidget_left->setFullScreen(true);
    }
}

void Player::setTrackInfo(const QString &info)
{
    m_trackInfo = info;

    if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {
        if (!m_statusInfo.isEmpty())
            setWindowTitle(QString("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
        else
            setWindowTitle(m_trackInfo);
    }
}

void Player::setStatusInfo(const QString &info)
{
    m_statusInfo = info;

    if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {
        if (!m_statusInfo.isEmpty())
            setWindowTitle(QString("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
        else
            setWindowTitle(m_trackInfo);
    }
}
void Player::calibrateFinished(){
    this->calibrationButton->setEnabled(true);
    ImageViewer* imageViewer = new ImageViewer("/media/ftc/H/DIP/GUI/out/customize_test_frame.jpg");
    connect(imageViewer, &QDialog::accepted, [this](){
        QMessageBox::information(this, "INFO", "You choose YES");
//        this->fileUploader->setFileName("/media/ftc/H/DIP/GUI/out/human_trans.pkl");
//        this->fileUploader->startTransfer();
    });
    imageViewer->show();
    imageViewer->exec();
    qDebug("Hello");
}
void Player::displayErrorMessage()
{
    if (m_player_left->error() == QMediaPlayer::NoError)
        return;
    setStatusInfo(m_player_left->errorString());
}

void Player::updateDurationInfo(qint64 currentInfo)
{
    QString tStr;
    if (currentInfo || m_duration) {
        QTime currentTime((currentInfo /1000/ 3600) % 60, (currentInfo / 60/1000) % 60,
            (currentInfo /1000)% 60, (currentInfo ) % 1000);
        QTime totalTime((m_duration / 3600/1000) % 60, (m_duration / 60/1000) % 60,
            (m_duration /1000)% 60, (m_duration ) % 1000);
        QString format = "mm:ss";
        if (m_duration/1000 > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    m_labelDuration->setText(tStr);
}

void Player::audioOutputChanged(int index)
{
    auto device = m_audioOutputCombo->itemData(index).value<QAudioDevice>();
    m_player_left->audioOutput()->setDevice(device);
}
void Player::calibrate(){
    qDebug("Push calibrate!");
    /// TODO:
    /// pack the python program to exe file
#ifdef Q_OS_WIN32
    QFileInfo getPlaceExe("./get_place.exe");
    if(!getPlaceExe.isFile()){
        textBrowser->append("For windows users, please pack ./GUI/get_place.py to get_place.exe then put it to ./");
        return;
    }
    calibrateProgress->setProgram("./get_place.exe");
#elif defined(Q_OS_LINUX)
    calibrateProgress->setProgram("/usr/local/anaconda3/bin/python");
    calibrateProgress->setArguments(QStringList("./GUI/get_place.py"));
#else
    // not support
    textBrowser->append("We do not support other os except WINDOWS or LINUX");
    return false;
#endif
    calibrateProgress->start();
    calibrationButton->setEnabled(false);
}
void Player::saveJson(){
    QMap<int, QMap<int, FootballPlayer>>* frames = frames1;
    QString jsonName = configMessage->json1Path;
    if(currentTeamViewed == 2) {
        frames = frames2;
        jsonName = configMessage->json2Path;
    }
    qDebug() << "click save json!\n";
    QMap<int, FootballPlayer> frameModel;
    for(int row = 1; row<currentModel->rowCount(); row++){
        FootballPlayer player;
        int id = currentModel->itemData(currentModel->index(row, 0)).cbegin()->toString().toInt();
        QString verbString = currentModel->itemData(currentModel->index(row, 1)).cbegin()->toString();
        while (verbString.endsWith(' ')) {
            verbString.chop(1);
        }
        player.verb = verbString.split(' ');
        player.skill = currentModel->itemData(currentModel->index(row, 2)).cbegin()->toString();
        player.fight = currentModel->itemData(currentModel->index(row, 3)).cbegin()->toString();
        frameModel[id] = player;
    }
    (*frames)[currentFrameViewed] = frameModel;
    try{
        saveFrameJson(frames, jsonName);
    }
    catch(std::runtime_error e){
        textBrowser->append(e.what());
        return;
    }

    qDebug() << "Save json " << currentFrameViewed << " to " << jsonName << '\n';
    textBrowser->append(tr("Save json %1 to %2!").arg(QString::number(currentTeamViewed), jsonName));
}
QString Player::getTimeInfo(qint64 currentInfo){
    QString tStr;
    if (currentInfo || m_duration) {
        QTime currentTime((currentInfo / 3600/1000) % 60, (currentInfo / 60/1000) % 60,
            (currentInfo /1000)% 60, (currentInfo ) % 1000);
        QTime totalTime((m_duration / 3600/1000) % 60, (m_duration / 60/1000) % 60,
            (m_duration /1000)% 60, (m_duration ) % 1000);
        QString format = "mm:ss";
        if (m_duration/1000 > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
    }
    return tStr;
}
QAbstractItemModel* Player::createPlayerModel(int frame, int team){
    QStandardItemModel* model = new QStandardItemModel(0, 4, this);
    model->setHeaderData(0, Qt::Horizontal, tr("id"));
    model->setHeaderData(1, Qt::Horizontal, tr("verb"));
    model->setHeaderData(2, Qt::Horizontal, tr("skill"));
    model->setHeaderData(3, Qt::Horizontal, tr("fight"));
    const QMap<int, QMap<int, FootballPlayer>>* frames;
    if(team == 1){
        frames = frames1;
    }
    else if(team == 2){
        frames = frames2;
    }
    else{
        textBrowser->append("Team is not specified!");
        frames = frames1;
    }
    auto iterFrames = frames->find(frame);
    if(iterFrames!=frames->cend()){
        for(auto iterPlayer=iterFrames->cbegin();iterPlayer!=iterFrames->cend();iterPlayer++){
            model->insertRow(0);
            model->setData(model->index(0,0), QString::number(iterPlayer.key())); //id
            const FootballPlayer& player = iterPlayer.value();
            QString verbString;
            for(auto p = player.verb.cbegin();p!=player.verb.cend();p++){
                verbString += *p;
                verbString.append(' ');
            }
            model->setData(model->index(0,1), verbString); //verb
            model->setData(model->index(0,2), player.skill); //skill
            model->setData(model->index(0,3), player.fight); //fight
        }
    }
    return model;
}
int Player::getCurrentFrame(){
//  TODO: there still exist a bug if somebody click pause before load json
    double r = (double)m_player_right->position() / m_duration;
    int currentFrame = r * (*(frames1->keyEnd()));
    qDebug() << tr("%1 / %2, frame = %3, totFrames = %4").arg(
        QString::number(m_player_right->position()), QString::number(m_duration),
        QString::number(currentFrame), QString::number(*(frames1->keyEnd()))
    );
    return currentFrame;
}
void Player::loadPlayerModel(){
    if(this->frames1 == nullptr){
        bool ret = loadJson();
        if(!ret){
            textBrowser->append("Load Player Model Failed!");
            return;
        }
    }
    int frame = getCurrentFrame();
    int team = currentSource;
    currentModel = this->createPlayerModel(frame, team);
    this->setPlayerModel(currentModel);
    this->currentFrameViewed = frame;
    this->currentTeamViewed = team;
    textBrowser->append(tr("Load Information from Frame %1!").arg(QString::number(frame)));
}
