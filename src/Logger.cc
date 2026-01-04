#include "Logger.hh"
#include "G4SystemOfUnits.hh"
#include <ctime>
#include <iomanip>
#include <sstream>

// Initialisation du singleton
Logger* Logger::fInstance = nullptr;

Logger::Logger()
: fEnabled(true),
  fEchoToConsole(false),
  fFilename("output.log")
{}

Logger::~Logger()
{
    Close();
}

Logger* Logger::GetInstance()
{
    if (fInstance == nullptr) {
        fInstance = new Logger();
    }
    return fInstance;
}

void Logger::Open(const G4String& filename)
{
    if (fLogFile.is_open()) {
        fLogFile.close();
    }
    
    fFilename = filename;
    fLogFile.open(filename, std::ios::out);
    
    if (fLogFile.is_open()) {
        G4cout << "Logger: Output redirected to " << filename << G4endl;
        
        // Écrire un header avec la date/heure
        std::time_t now = std::time(nullptr);
        std::tm* ltm = std::localtime(&now);
        
        fLogFile << "╔═══════════════════════════════════════════════════════════════════╗\n";
        fLogFile << "║            PUITS COURONNE - DIAGNOSTIC LOG                        ║\n";
        fLogFile << "║            " << std::put_time(ltm, "%Y-%m-%d %H:%M:%S") 
                 << "                               ║\n";
        fLogFile << "╚═══════════════════════════════════════════════════════════════════╝\n";
        fLogFile << "\n";
        fLogFile.flush();
    } else {
        G4cerr << "Logger: ERROR - Could not open " << filename << G4endl;
    }
}

void Logger::Close()
{
    if (fLogFile.is_open()) {
        // Écrire un footer
        std::time_t now = std::time(nullptr);
        std::tm* ltm = std::localtime(&now);
        
        fLogFile << "\n";
        fLogFile << "╔═══════════════════════════════════════════════════════════════════╗\n";
        fLogFile << "║            END OF LOG - " << std::put_time(ltm, "%Y-%m-%d %H:%M:%S") 
                 << "                    ║\n";
        fLogFile << "╚═══════════════════════════════════════════════════════════════════╝\n";
        
        fLogFile.close();
        G4cout << "Logger: Log file closed." << G4endl;
    }
}

void Logger::Log(const G4String& message)
{
    if (!fEnabled) return;
    
    if (fLogFile.is_open()) {
        fLogFile << message;
        fLogFile.flush();
    }
    
    if (fEchoToConsole) {
        G4cout << message;
    }
}

void Logger::LogLine(const G4String& message)
{
    if (!fEnabled) return;
    
    if (fLogFile.is_open()) {
        fLogFile << message << "\n";
        fLogFile.flush();
    }
    
    if (fEchoToConsole) {
        G4cout << message << G4endl;
    }
}

void Logger::LogSeparator(char c, int length)
{
    if (!fEnabled) return;
    
    std::string separator(length, c);
    LogLine(separator);
}

void Logger::LogHeader(const G4String& title)
{
    if (!fEnabled) return;
    
    std::ostringstream oss;
    oss << "\n";
    oss << "======================================================================\n";
    oss << "  " << title << "\n";
    oss << "======================================================================\n";
    
    if (fLogFile.is_open()) {
        fLogFile << oss.str();
        fLogFile.flush();
    }
    
    if (fEchoToConsole) {
        G4cout << oss.str();
    }
}
