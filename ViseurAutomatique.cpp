#include "ViseurAutomatique.h"
#include <Arduino.h>

// Constructeur

// p1 à p4 : broches IN1 à IN4 du ULN2003

// distanceRef : référence à la distance détectée

ViseurAutomatique::ViseurAutomatique(int p1, int p2, int p3, int p4, float& distanceRef)
  : _stepper(AccelStepper::FULL4WIRE, p1, p3, p2, p4),
    _distance(distanceRef) {
  _stepper.setMaxSpeed(1000);
  _stepper.setAcceleration(1000);
  _stepper.setSpeed(1000);
}


// Doit être appelée continuellement dans loop()

void ViseurAutomatique::update() {
  _currentTime = millis();
  switch (_etat) {
    case INACTIF: _inactifState(_currentTime); break;
    case SUIVI: _suiviState(_currentTime); break;
    case REPOS: _reposState(_currentTime); break;
  }
}


// Régle l’angle minimal (position la plus à gauche)

void ViseurAutomatique::setAngleMin(float angle) {
  _angleMin = angle;
}


// Régle l’angle maximal (position la plus à droite)

void ViseurAutomatique::setAngleMax(float angle) {
  _angleMax = angle;
}


// Régle le nombre de pas par tour du moteur

void ViseurAutomatique::setPasParTour(int steps) {
  _stepsPerRev = steps;
}


// Définit la distance minimale à partir de laquelle le viseur commence à suivre

void ViseurAutomatique::setDistanceMinSuivi(float distanceMin) {
  _distanceMinSuivi = distanceMin;
}


// Définit la distance maximale jusqu'où le viseur peut suivre

void ViseurAutomatique::setDistanceMaxSuivi(float distanceMax) {
  _distanceMaxSuivi = distanceMax;
}


// Retourne l’angle actuel du viseur

float ViseurAutomatique::getAngle() const {
  //currentAngle = myStepper.currentPosition() / stepsByDegree + 1;
  //stepsByDegree = 2038.0 / 360.0;
  //long steps = _stepper.currentPosition();
  return (float)_stepper.currentPosition() / (_stepsPerRev / 360.0);
}


// Active le viseur en le mettant en état repos

void ViseurAutomatique::activer() {
  _etat = REPOS;
  _stepper.enableOutputs();
}


// Désactive le viseur en le mettant en état inactif

void ViseurAutomatique::desactiver() {
  _etat = INACTIF;
  _stepper.disableOutputs();
}


// Retourne l’état actuel du viseur sous forme de texte

const char* ViseurAutomatique::getEtatTexte() const {
  //retourne un nombre
  //return _etat;
  switch (_etat) {
    case INACTIF: return "INACTIF";
    case SUIVI: return "SUIVI";
    case REPOS: return "REPOS";
  }
}

// États

void ViseurAutomatique::_inactifState(unsigned long cT) {
  _stepper.disableOutputs();
}

void ViseurAutomatique::_suiviState(unsigned long cT) {
  float angle = (_distance - _distanceMinSuivi) * (_angleMax - _angleMin) / (_distanceMaxSuivi - _distanceMinSuivi) + _angleMin;
  angle = constrain(angle, _angleMin, _angleMax);
  long target = _angleEnSteps(angle);
  _stepper.moveTo(target);
  _stepper.run();
  if (_stepper.distanceToGo() == 0) {
    _stepper.disableOutputs();
  }
  // _stepper.moveTo(target);
  // _stepper.run();
  // if (_stepper.distanceToGo() == 0) {
  //   _stepper.disableOutputs();  // OK maintenant
  // } else {
  //   _stepper.enableOutputs();  // utile si tu avais désactivé avant
  // }
  // angle = map(distance, minDistance, maxDistance, minDegree, maxDegree);
  //   steps = angle * stepsByDegree;

  //   if (myStepper.distanceToGo() == 0) {
  //     myStepper.moveTo(steps);
  //   }

  //   myStepper.run();

  //   if (myStepper.distanceToGo() == 0) {
  //     myStepper.disableOutputs();
  //   }
}

void ViseurAutomatique::_reposState(unsigned long cT) {
  float angleRepos = (_angleMin + _angleMax) / 2.0;
  long target = _angleEnSteps(angleRepos);
  _stepper.moveTo(target);
  _stepper.run();
  if (_distance >= _distanceMinSuivi && _distance <= _distanceMaxSuivi) {
    _etat = SUIVI;
  }
}


long ViseurAutomatique::_angleEnSteps(float angle) const {
  //long a verifier
  return (long)(_stepsPerRev * angle / 360);
}