#include "EnergyMonitor.h"
#include "PinDefinitions.h"
#include <Arduino.h>


EnergyMonitor::EnergyMonitor(SystemStatus& systemStatus) 
    : systemStatus(systemStatus), current1(0), current2(0), voltage(220.0), power(0), energy(0), cost(0), previousMillis(0), isPhaseToPhase(false) {}

void EnergyMonitor::setup() {
    pinMode(ACS712_PIN1, INPUT);
    pinMode(ACS712_PIN2, INPUT);
}

void EnergyMonitor::loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) { // Atualiza a cada segundo
        previousMillis = currentMillis;
        
        current1 = readCurrent(ACS712_PIN1);
        current2 = readCurrent(ACS712_PIN2);
        
        detectConfiguration(); // Detecta se é fase-fase ou fase-neutro
        
        if (isPhaseToPhase) {
            // Configuração fase-fase
            power = (current1 + current2) * voltage / 2.0; // Corrente média dos dois sensores
        } else {
            // Configuração fase-neutro
            power = (current1 + current2) * voltage; // Soma das correntes vezes a tensão de 220V
        }
        
        energy += power / 3600.0; // Convertendo watts para watt-horas
        calculateCost();
        
        // Atualiza os valores no SystemStatus
        systemStatus.power = power;
        //Serial.println("Power: " + String(power) + "W");
        systemStatus.energy = energy;
       // Serial.println("Energy: " + String(energy) + "Wh");
        systemStatus.cost = cost;
       // Serial.println("Cost: R$" + String(cost));
    }
}

float EnergyMonitor::readCurrent(int sensorPin) {
    int sensorValue = analogRead(sensorPin);
    float voltage = (sensorValue / 4095.0) * 3.3; // Convertendo para a tensão de entrada
    float current = (voltage - 2.5) / 0.066; // Para ACS712 30A, sensibilidade é 66mV/A
    return current;
}

void EnergyMonitor::calculateCost() {
    cost = (energy / 1000.0) * systemStatus.kWhCost; // Convertendo Wh para kWh e multiplicando pelo custo de kWh definido em systemStatus
}

void EnergyMonitor::detectConfiguration() {
    if (current1 > 0 && current2 > 0) {
        isPhaseToPhase = true; // Ambos os sensores estão medindo corrente
    } else {
        isPhaseToPhase = false; // Apenas um sensor está medindo corrente
    }
}
