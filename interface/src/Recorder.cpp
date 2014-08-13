//
//  Recorder.cpp
//
//
//  Created by Clement on 8/7/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <GLMHelpers.h>

#include "Recorder.h"

void RecordingFrame::setBlendshapeCoefficients(QVector<float> blendshapeCoefficients) {
    _blendshapeCoefficients = blendshapeCoefficients;
}

void RecordingFrame::setJointRotations(QVector<glm::quat> jointRotations) {
    _jointRotations = jointRotations;
}

void RecordingFrame::setTranslation(glm::vec3 translation) {
    _translation = translation;
}

void RecordingFrame::setRotation(glm::quat rotation) {
    _rotation = rotation;
}

void RecordingFrame::setScale(float scale) {
    _scale = scale;
}

void RecordingFrame::setHeadRotation(glm::quat headRotation) {
    _headRotation = headRotation;
}

void RecordingFrame::setLeanSideways(float leanSideways) {
    _leanSideways = leanSideways;
}

void RecordingFrame::setLeanForward(float leanForward) {
    _leanForward = leanForward;
}

void Recording::addFrame(int timestamp, RecordingFrame &frame) {
    _timestamps << timestamp;
    _frames << frame;
}

void Recording::clear() {
    _timestamps.clear();
    _frames.clear();
}

Recorder::Recorder(AvatarData* avatar) :
    _recording(new Recording()),
    _avatar(avatar)
{
}

bool Recorder::isRecording() const {
    return _timer.isValid();
}

qint64 Recorder::elapsed() const {
    if (isRecording()) {
        return _timer.elapsed();
    } else {
        return 0;
    }
}

void Recorder::startRecording() {
    qDebug() << "Recorder::startRecording()";
    _recording->clear();
    _timer.start();
    
    RecordingFrame frame;
    frame.setBlendshapeCoefficients(_avatar->getHeadData()->getBlendshapeCoefficients());
    frame.setJointRotations(_avatar->getJointRotations());
    frame.setTranslation(_avatar->getPosition());
    frame.setRotation(_avatar->getOrientation());
    frame.setScale(_avatar->getTargetScale());
    
    const HeadData* head = _avatar->getHeadData();
    glm::quat rotation = glm::quat(glm::radians(glm::vec3(head->getFinalPitch(),
                                                          head->getFinalYaw(),
                                                          head->getFinalRoll())));
    frame.setHeadRotation(rotation);
    frame.setLeanForward(_avatar->getHeadData()->getLeanForward());
    frame.setLeanSideways(_avatar->getHeadData()->getLeanSideways());
    
    _recording->addFrame(0, frame);
}

void Recorder::stopRecording() {
    qDebug() << "Recorder::stopRecording()";
    _timer.invalidate();
    
    qDebug().nospace() << "Recorded " << _recording->getFrameNumber() << " during " << _recording->getLength() << " msec (" << _recording->getFrameNumber() / (_recording->getLength() / 1000.0f) << " fps)";
}

void Recorder::saveToFile(QString file) {
    if (_recording->isEmpty()) {
        qDebug() << "Cannot save recording to file, recording is empty.";
    }
    
    writeRecordingToFile(*_recording, file);
}

void Recorder::record() {
    if (isRecording()) {
        const RecordingFrame& referenceFrame = _recording->getFrame(0);
        RecordingFrame frame;
        frame.setBlendshapeCoefficients(_avatar->getHeadData()->getBlendshapeCoefficients());
        frame.setJointRotations(_avatar->getJointRotations());
        frame.setTranslation(_avatar->getPosition() - referenceFrame.getTranslation());
        frame.setRotation(glm::inverse(referenceFrame.getRotation()) * _avatar->getOrientation());
        frame.setScale(_avatar->getTargetScale() / referenceFrame.getScale());
        
        
        const HeadData* head = _avatar->getHeadData();
        glm::quat rotation = glm::quat(glm::radians(glm::vec3(head->getFinalPitch(),
                                                              head->getFinalYaw(),
                                                              head->getFinalRoll())));
        frame.setHeadRotation(rotation);
        frame.setLeanForward(_avatar->getHeadData()->getLeanForward());
        frame.setLeanSideways(_avatar->getHeadData()->getLeanSideways());
        
        _recording->addFrame(_timer.elapsed(), frame);
    }
}

Player::Player(AvatarData* avatar) :
    _recording(new Recording()),
    _avatar(avatar)
{
}

bool Player::isPlaying() const {
    return _timer.isValid();
}

qint64 Player::elapsed() const {
    if (isPlaying()) {
        return _timer.elapsed();
    } else {
        return 0;
    }
}

QVector<float> Player::getBlendshapeCoefficients() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(_currentFrame - 1).getBlendshapeCoefficients();
        }
        
        return _recording->getFrame(_currentFrame).getBlendshapeCoefficients();
    }
    qWarning() << "Incorrect use of Player::getBlendshapeCoefficients()";
    return QVector<float>();
}

QVector<glm::quat> Player::getJointRotations() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(_currentFrame - 1).getJointRotations();
        }
        
        return _recording->getFrame(_currentFrame).getJointRotations();
    }
    qWarning() << "Incorrect use of Player::getJointRotations()";
    return QVector<glm::quat>();
}

glm::vec3 Player::getPosition() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(0).getTranslation() +
                   _recording->getFrame(_currentFrame - 1).getTranslation();
        }
        if (_currentFrame == 0) {
            return _recording->getFrame(_currentFrame).getTranslation();
        }
        
        return _recording->getFrame(0).getTranslation() +
               _recording->getFrame(_currentFrame).getTranslation();
    }
    qWarning() << "Incorrect use of Player::getTranslation()";
    return glm::vec3();
}

glm::quat Player::getRotation() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(0).getRotation() *
                   _recording->getFrame(_currentFrame - 1).getRotation();
        }
        if (_currentFrame == 0) {
            return _recording->getFrame(_currentFrame).getRotation();
        }
        
        return _recording->getFrame(0).getRotation() *
               _recording->getFrame(_currentFrame).getRotation();
    }
    qWarning() << "Incorrect use of Player::getRotation()";
    return glm::quat();
}

float Player::getScale() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(0).getScale() *
                   _recording->getFrame(_currentFrame - 1).getScale();
        }
        if (_currentFrame == 0) {
            return _recording->getFrame(_currentFrame).getScale();
        }
        
        return _recording->getFrame(0).getScale() *
               _recording->getFrame(_currentFrame).getScale();
    }
    qWarning() << "Incorrect use of Player::getScale()";
    return 1.0f;
}

glm::quat Player::getHeadRotation() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(0).getHeadRotation() *
                   _recording->getFrame(_currentFrame - 1).getHeadRotation();
        }
        if (_currentFrame == 0) {
            return _recording->getFrame(_currentFrame).getHeadRotation();
        }
        
        return _recording->getFrame(0).getHeadRotation() *
               _recording->getFrame(_currentFrame).getHeadRotation();
    }
    qWarning() << "Incorrect use of Player::getHeadRotation()";
    return glm::quat();
}

float Player::getLeanSideways() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(_currentFrame - 1).getLeanSideways();
        }
        
        return _recording->getFrame(_currentFrame).getLeanSideways();
    }
    qWarning() << "Incorrect use of Player::getLeanSideways()";
    return 0.0f;
}

float Player::getLeanForward() {
    computeCurrentFrame();
    if (_currentFrame >= 0 && _currentFrame <= _recording->getFrameNumber()) {
        if (_currentFrame == _recording->getFrameNumber()) {
            return _recording->getFrame(_currentFrame - 1).getLeanForward();
        }
        
        return _recording->getFrame(_currentFrame).getLeanForward();
    }
    qWarning() << "Incorrect use of Player::getLeanForward()";
    return 0.0f;
}


void Player::startPlaying() {
    if (_recording && _recording->getFrameNumber() > 0) {
        qDebug() << "Recorder::startPlaying()";
        _timer.start();
        _currentFrame = 0;
    }
}

void Player::stopPlaying() {
    qDebug() << "Recorder::stopPlaying()";
    _timer.invalidate();
}

void Player::loadFromFile(QString file) {
    if (_recording) {
        _recording->clear();
    } else {
        _recording = RecordingPointer(new Recording());
    }
    readRecordingFromFile(*_recording, file);
}

void Player::loadRecording(RecordingPointer recording) {
    _recording = recording;
}

void Player::play() {
    qDebug() << "Playing " << _timer.elapsed() / 1000.0f;
    computeCurrentFrame();
    if (_currentFrame < 0 || _currentFrame >= _recording->getFrameNumber()) {
        // If it's the end of the recording, stop playing
        stopPlaying();
        return;
    }
    if (_currentFrame == 0) {
        _avatar->setPosition(_recording->getFrame(_currentFrame).getTranslation());
        _avatar->setOrientation(_recording->getFrame(_currentFrame).getRotation());
        _avatar->setTargetScale(_recording->getFrame(_currentFrame).getScale());
        _avatar->setJointRotations(_recording->getFrame(_currentFrame).getJointRotations());
        HeadData* head = const_cast<HeadData*>(_avatar->getHeadData());
        head->setBlendshapeCoefficients(_recording->getFrame(_currentFrame).getBlendshapeCoefficients());
        // TODO
        // BODY: Joint Rotations
    } else {
        _avatar->setPosition(_recording->getFrame(0).getTranslation() +
                             _recording->getFrame(_currentFrame).getTranslation());
        _avatar->setOrientation(_recording->getFrame(0).getRotation() *
                                _recording->getFrame(_currentFrame).getRotation());
        _avatar->setTargetScale(_recording->getFrame(0).getScale() *
                                _recording->getFrame(_currentFrame).getScale());
        _avatar->setJointRotations(_recording->getFrame(_currentFrame).getJointRotations());
        HeadData* head = const_cast<HeadData*>(_avatar->getHeadData());
        head->setBlendshapeCoefficients(_recording->getFrame(_currentFrame).getBlendshapeCoefficients());
        // TODO
        // BODY: Joint Rotations
    }
}

void Player::computeCurrentFrame() {
    if (!isPlaying()) {
        qDebug() << "Not Playing";
        _currentFrame = -1;
        return;
    }
    if (_currentFrame < 0) {
        qDebug() << "Reset to 0";
        _currentFrame = 0;
    }
    
    while (_currentFrame < _recording->getFrameNumber() &&
           _recording->getFrameTimestamp(_currentFrame) < _timer.elapsed()) {
        ++_currentFrame;
    }
}

void writeRecordingToFile(Recording& recording, QString file) {
    // TODO
    qDebug() << "Writing recording to " << file;
}

Recording& readRecordingFromFile(Recording& recording, QString file) {
    // TODO
    qDebug() << "Reading recording from " << file;
    return recording;
}