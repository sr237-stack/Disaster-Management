#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <string>
#include <sstream>

// ================================================================
// PROJECT: Disaster Monitoring and Early Warning System
// ================================================================

namespace DisasterCore {
namespace Sensors {

// Unit I: inline utility function for a simple risk calculation.
inline double calculateHazardIndex(double rainfall, double slopeAngle) {
    return (rainfall * 0.6) + (slopeAngle * 0.4);
}

// Unit I: overloaded function for a different sensor combination.
inline double calculateHazardIndex(double soilMoisture, double vibration,
                                   double riverLevel) {
    return (soilMoisture * 0.3) + (vibration * 0.4) + (riverLevel * 0.3);
}

// Unit I: call by reference updates coordinate values without copying.
inline void updateCoordinates(double &latitude, double &longitude) {
    latitude += 0.001;
    longitude += 0.001;
}

// Unit I: return by reference allows the latest reading to be modified.
double& latestReading(std::vector<double> &readings) {
    if (readings.empty()) readings.push_back(0.0);
    return readings.back();
}

class SensorStation;

// Unit I: friend class can inspect private raw sensor metrics.
class TelemetryAuditor {
public:
    void inspect(const SensorStation &station);
};

// Unit II: encapsulation, dynamic memory, static data and operator overloading.
class SensorStation {
private:
    int stationId;
    std::string stationName;
    double *rawMetrics;
    int metricCount;
    double latitude, longitude;

    friend class TelemetryAuditor;

public:
    static int activeStations;
    static int globalAlertLevel;

    SensorStation(int id = 0, const std::string &name = "Station", int count = 3)
        : stationId(id), stationName(name), metricCount(count),
          latitude(28.6), longitude(77.2) {
        if (metricCount < 1) metricCount = 1;
        rawMetrics = new double[metricCount]();
        ++activeStations;
    }

    // Unit II: deep-copy constructor prevents shared ownership of dynamic memory.
    SensorStation(const SensorStation &other)
        : stationId(other.stationId), stationName(other.stationName),
          metricCount(other.metricCount), latitude(other.latitude),
          longitude(other.longitude) {
        rawMetrics = new double[metricCount];
        for (int i = 0; i < metricCount; ++i) rawMetrics[i] = other.rawMetrics[i];
        ++activeStations;
    }

    // Unit II: destructor releases dynamically allocated memory.
    ~SensorStation() {
        delete[] rawMetrics;
        --activeStations;
    }

    void setMetric(int index, double value) {
        if (index >= 0 && index < metricCount) rawMetrics[index] = value;
    }
    double getMetric(int index) const {
        if (index >= 0 && index < metricCount) return rawMetrics[index];
        return 0.0;
    }
    int getId() const { return stationId; }
    std::string getName() const { return stationName; }
    void setLocation(double lat, double lon) {
        latitude = lat; longitude = lon;
        updateCoordinates(latitude, longitude); // call by reference
    }

    static void showNetworkStatus() {
        std::cout << "Active sensor stations: " << activeStations
                  << " | Global alert level: " << globalAlertLevel << '\n';
    }

    // Unit II: + combines the number of metrics from two stations.
    SensorStation operator+(const SensorStation &other) const {
        SensorStation combined(stationId, stationName + "+" + other.stationName,
                               metricCount + other.metricCount);
        for (int i = 0; i < metricCount; ++i) combined.rawMetrics[i] = rawMetrics[i];
        for (int j = 0; j < other.metricCount; ++j)
            combined.rawMetrics[metricCount + j] = other.rawMetrics[j];
        return combined;
    }

    // Unit II: > compares the average raw metric values.
    bool operator>(const SensorStation &other) const {
        double a = 0, b = 0;
        for (int i = 0; i < metricCount; ++i) a += rawMetrics[i];
        for (int i = 0; i < other.metricCount; ++i) b += other.rawMetrics[i];
        return (a / metricCount) > (b / other.metricCount);
    }

    // Unit II: stream insertion and extraction operators.
    friend std::ostream& operator<<(std::ostream &out, const SensorStation &s) {
        out << "Station ID: " << s.stationId << " | Name: " << s.stationName
            << " | Metrics: ";
        for (int i = 0; i < s.metricCount; ++i) out << s.rawMetrics[i] << ' ';
        return out;
    }
    friend std::istream& operator>>(std::istream &in, SensorStation &s) {
        std::cout << "Enter " << s.metricCount << " sensor readings: ";
        for (int i = 0; i < s.metricCount; ++i) in >> s.rawMetrics[i];
        return in;
    }
};

int SensorStation::activeStations = 0;
int SensorStation::globalAlertLevel = 0;

// Unit II: additional abstracted model entity for buffered telemetry.
class TelemetryBuffer {
private:
    std::vector<double> samples;
public:
    void addReading(double value) { samples.push_back(value); }
    const std::vector<double>& getReadings() const { return samples; }
    double average() const {
        if (samples.empty()) return 0.0;
        double sum = 0.0;
        for (double value : samples) sum += value;
        return sum / samples.size();
    }
};

void TelemetryAuditor::inspect(const SensorStation &station) {
    std::cout << "[Auditor - private raw data access] ";
    for (int i = 0; i < station.metricCount; ++i)
        std::cout << station.rawMetrics[i] << ' ';
    std::cout << '\n';
}

} // namespace Sensors

namespace Alerts {

// Unit IV: custom exceptions derived from std::exception.
class SensorFailureException : public std::exception {
public:
    const char* what() const noexcept override { return "Sensor failure detected."; }
};
class TelemetryChecksumMismatch : public std::exception {
public:
    const char* what() const noexcept override { return "Telemetry checksum mismatch."; }
};
class ThresholdOverflowException : public std::exception {
public:
    const char* what() const noexcept override { return "Hazard threshold exceeded!"; }
};

// Unit II: encapsulated alert actuator model.
class EarlyWarningSiren {
private:
    bool active;
public:
    EarlyWarningSiren() : active(false) {}
    void activate() { active = true; }
    void deactivate() { active = false; }
    bool isActive() const { return active; }
};

// Unit III: independent component used through containership/composition.
class PowerSubsystem {
    int batteryPercent;
public:
    explicit PowerSubsystem(int charge = 100) : batteryPercent(charge) {}
    void showPower() const { std::cout << "Power subsystem battery: " << batteryPercent << "%\n"; }
};

// Unit III: abstract base class. It contains an independent PowerSubsystem.
class BaseSensorNode {
protected:
    int nodeId;
    PowerSubsystem power; // composition: a sensor node HAS-A power subsystem
public:
    explicit BaseSensorNode(int id) : nodeId(id), power(95) {}
    void showPowerStatus() const { power.showPower(); }
    virtual void readTelemetry() = 0;
    virtual double evaluateThreshold() const = 0;
    virtual ~BaseSensorNode() {} // virtual destructor for safe deletion
};

// Unit III: root class in the diamond hierarchy.
class DisasterTelemetryDevice : public BaseSensorNode {
protected:
    std::vector<double> readings;
public:
    explicit DisasterTelemetryDevice(int id) : BaseSensorNode(id) {}
    void readTelemetry() override {
        readings = {42.0, 68.0, 35.0};
        std::cout << "Device " << nodeId << " collected telemetry.\n";
    }
    double evaluateThreshold() const override {
        if (readings.empty()) return 0.0;
        double total = 0;
        for (double value : readings) total += value;
        return total / readings.size();
    }
    virtual ~DisasterTelemetryDevice() {}
};

// Unit III: virtual inheritance avoids duplicate DisasterTelemetryDevice bases.
class SolarPoweredNode : virtual public DisasterTelemetryDevice {
public:
    explicit SolarPoweredNode(int id) : DisasterTelemetryDevice(id) {}
    void solarStatus() const { std::cout << "Solar power: ON\n"; }
};
class WirelessLoRaTransceiver : virtual public DisasterTelemetryDevice {
public:
    explicit WirelessLoRaTransceiver(int id) : DisasterTelemetryDevice(id) {}
    void transmit() const { std::cout << "LoRa transmission: ACTIVE\n"; }
};

// Unit III: derived leaf combines both intermediate classes (diamond pattern).
class AutonomousHybridProbe : public SolarPoweredNode, public WirelessLoRaTransceiver {
public:
    explicit AutonomousHybridProbe(int id)
        : DisasterTelemetryDevice(id), SolarPoweredNode(id),
          WirelessLoRaTransceiver(id) {}
    void readTelemetry() override {
        readings = {72.0, 81.0, 66.0};
        std::cout << "Hybrid probe " << nodeId << " collected sensor data.\n";
    }
    double evaluateThreshold() const override {
        if (readings.empty()) return 0.0;
        double total = 0;
        for (double value : readings) total += value;
        return total / readings.size();
    }
    void showComponents() const { solarStatus(); transmit(); showPowerStatus(); }
};

// Unit IV: simple binary record (fixed-size fields make serialization predictable).
struct IncidentRecord {
    int stationId;
    double riskIndex;
    char status[20];
};

void saveIncidentAtomically(const IncidentRecord &record, const std::string &fileName) {
    const std::string tempName = fileName + ".tmp";
    std::ifstream oldFile(fileName, std::ios::binary);
    std::fstream tempFile(tempName, std::ios::in | std::ios::out |
                                      std::ios::binary | std::ios::trunc);
    if (!tempFile) throw SensorFailureException();

    if (oldFile) {
        char buffer[512];
        while (oldFile.read(buffer, sizeof(buffer)) || oldFile.gcount() > 0)
            tempFile.write(buffer, oldFile.gcount());
    }

    // Remember the last valid position. If writing fails, seekp() rolls
    // the temporary stream back to this commit point; the temp file is discarded.
    std::streampos commitPosition = tempFile.tellp();
    tempFile.write(reinterpret_cast<const char*>(&record), sizeof(record));
    if (!tempFile) {
        tempFile.clear();
        tempFile.seekp(commitPosition); // transactional rollback position
        tempFile.close();
        oldFile.close();
        std::remove(tempName.c_str());
        throw SensorFailureException();
    }
    tempFile.flush();
    tempFile.close();
    oldFile.close();
    if (!tempFile) {
        std::remove(tempName.c_str());
        throw SensorFailureException();
    }

    // Replace the old log only after the complete temporary log is written.
    std::remove(fileName.c_str());
    if (std::rename(tempName.c_str(), fileName.c_str()) != 0) {
        std::remove(tempName.c_str());
        throw SensorFailureException();
    }
}

std::vector<IncidentRecord> loadIncidents(const std::string &fileName) {
    std::vector<IncidentRecord> records;
    std::fstream file(fileName, std::ios::in | std::ios::binary);
    if (!file) return records;
    IncidentRecord record{};
    while (true) {
        std::streampos position = file.tellg(); // seekg/tellg for reading position
        file.read(reinterpret_cast<char*>(&record), sizeof(record));
        if (!file) {
            file.clear();
            file.seekg(position); // return to last complete record boundary
            break;
        }
        records.push_back(record);
    }
    return records;
}

// Unit IV: custom functor for priority-based sorting.
struct RiskDescending {
    bool operator()(const IncidentRecord &a, const IncidentRecord &b) const {
        return a.riskIndex > b.riskIndex;
    }
};

// Unit IV: user-defined stream manipulator for tabular output.
std::ostream& tableLine(std::ostream &out) {
    out << "-------------------------------------------------------------\n";
    return out;
}

} // namespace Alerts
} // namespace DisasterCore

int main() {
    using namespace DisasterCore::Sensors;
    using namespace DisasterCore::Alerts;

    std::cout << "\n===== DISASTER MONITORING & EARLY WARNING SYSTEM =====\n";

    // Unit II: dynamically allocated station objects and copy constructor demo.
    SensorStation *station1 = new SensorStation(101, "Himalayan-Alpha", 3);
    SensorStation station2(102, "River-Beta", 3);
    (*station1).setMetric(0, 78.0); // rainfall
    (*station1).setMetric(1, 82.0); // soil moisture
    (*station1).setMetric(2, 70.0); // seismic vibration
    station2.setMetric(0, 35.0);
    station2.setMetric(1, 40.0);
    station2.setMetric(2, 30.0);

    SensorStation copiedStation(*station1); // deep copy constructor
    // Demonstrate overloaded extraction operator using a test input stream.
    std::istringstream demoInput("35 40 30");
    demoInput >> station2;

    std::cout << "\n--- Sensor station details ---\n";
    std::cout << *station1 << '\n' << station2 << '\n';
    std::cout << "Station 1 has higher average readings than Station 2: "
              << ((*station1 > station2) ? "YES" : "NO") << '\n';

    SensorStation combined = (*station1) + station2;
    std::cout << "Combined station metrics: " << combined << '\n';

    TelemetryAuditor auditor;
    auditor.inspect(*station1); // friend class accesses private rawMetrics

    // Unit I: return by reference - update latest stored reading.
    std::vector<double> recentReadings = {25.0, 48.0, 61.0};
    latestReading(recentReadings) = 66.0;
    std::cout << "Latest updated reading: " << latestReading(recentReadings) << '\n';

    TelemetryBuffer buffer;
    buffer.addReading(42.0); buffer.addReading(68.0); buffer.addReading(35.0);
    std::cout << "Buffered telemetry average: " << buffer.average() << '\n';

    // Unit I: overloaded inline hazard functions.
    double riskA = calculateHazardIndex(78.0, 35.0);
    double riskB = calculateHazardIndex(82.0, 70.0, 68.0);
    double finalRisk = (riskA + riskB) / 2.0;
    std::cout << "Calculated hazard index: " << finalRisk << '\n';

    std::string status;
    if (finalRisk >= 70.0) status = "CRITICAL";
    else if (finalRisk >= 55.0) status = "HIGH";
    else if (finalRisk >= 35.0) status = "MODERATE";
    else status = "NORMAL";

    std::cout << "Hazard status: " << status << '\n';
    SensorStation::globalAlertLevel = (status == "CRITICAL") ? 3 :
                                      (status == "HIGH") ? 2 :
                                      (status == "MODERATE") ? 1 : 0;
    SensorStation::showNetworkStatus();

    // Unit III: runtime polymorphism through BaseSensorNode pointers.
    std::cout << "\n--- Polymorphic sensor sweep ---\n";
    std::vector<BaseSensorNode*> nodes;
    nodes.push_back(new AutonomousHybridProbe(501));
    for (BaseSensorNode *node : nodes) {
        node->readTelemetry();
        std::cout << "Average telemetry: " << node->evaluateThreshold() << '\n';
        AutonomousHybridProbe *probe = dynamic_cast<AutonomousHybridProbe*>(node);
        if (probe) probe->showComponents();
    }
    for (BaseSensorNode *node : nodes) delete node; // virtual destructor invoked

    // Unit IV: STL map and vector; exceptions for threshold conditions.
    std::map<std::string, double> thresholdMap;
    thresholdMap["HIGH"] = 55.0;
    thresholdMap["CRITICAL"] = 70.0;
    std::vector<IncidentRecord> incidents;
    IncidentRecord current{};
    current.stationId = station1->getId();
    current.riskIndex = finalRisk;
    std::strncpy(current.status, status.c_str(), sizeof(current.status) - 1);
    incidents.push_back(current);

    EarlyWarningSiren siren;
    try {
        if (finalRisk > 100.0) throw ThresholdOverflowException();
        if (finalRisk >= thresholdMap["CRITICAL"]) {
            siren.activate();
            std::cout << "\n*** REGIONAL SIREN ALERT: CRITICAL RISK ***\n";
        } else if (finalRisk >= thresholdMap["HIGH"]) {
            siren.activate();
            std::cout << "\n*** WARNING: HIGH RISK - MONITOR AREA ***\n";
        }
        else
            std::cout << "\nNo emergency siren required.\n";
    } catch (const std::exception &e) {
        std::cout << "Alert exception: " << e.what() << '\n';
    }
    std::cout << "Siren active: " << (siren.isActive() ? "YES" : "NO") << '\n';

    // Unit IV: sort incident records using a custom functor.
    std::sort(incidents.begin(), incidents.end(), RiskDescending());
    std::cout << tableLine;
    std::cout << std::left << std::setw(12) << "Station ID"
              << std::setw(15) << "Risk Index" << "Status\n";
    std::cout << tableLine;
    for (const auto &r : incidents)
        std::cout << std::left << std::setw(12) << r.stationId
                  << std::setw(15) << std::fixed << std::setprecision(2)
                  << r.riskIndex << r.status << '\n';
    std::cout << tableLine;

    // Unit IV: binary persistence and restoration.
    try {
        saveIncidentAtomically(current, "incident_log.dat");
        std::vector<IncidentRecord> saved = loadIncidents("incident_log.dat");
        std::cout << "Saved incident records restored from binary file: "
                  << saved.size() << '\n';
    } catch (const std::exception &e) {
        std::cout << "File handling error: " << e.what() << '\n';
    }

    delete station1; // destructor releases dynamic memory
    std::cout << "\nProgram completed successfully.\n";
    SensorStation::showNetworkStatus();
    return 0;
}
