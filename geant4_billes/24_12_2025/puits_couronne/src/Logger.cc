#include "Logger.hh"
#include "G4SystemOfUnits.hh"
#include <ctime>
#include <iomanip>

// Initialisation du singleton
Logger* Logger::fInstance = nullptr;

Logger::Logger()
: fEnabled(true),
  fEchoToConsole(false),  // Par défaut, pas d'écho sur console
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
        // Écrire l'en-tête du fichier
        time_t now = time(nullptr);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        fLogFile << "╔═══════════════════════════════════════════════════════════════════╗\n";
        fLogFile << "║            PUITS COURONNE - DIAGNOSTIC LOG                        ║\n";
        fLogFile << "║            " << timeStr << "                               ║\n";
        fLogFile << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
        fLogFile.flush();
        
        G4cout << "Logger: Output redirected to " << filename << G4endl;
    } else {
        G4cerr << "Logger: ERROR - Could not open " << filename << G4endl;
    }
}

void Logger::Close()
{
    if (fLogFile.is_open()) {
        // Écrire le pied de page
        time_t now = time(nullptr);
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        fLogFile << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
        fLogFile << "║            END OF LOG - " << timeStr << "                    ║\n";
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
    
    std::string sep(length, c);
    LogLine(sep);
}

void Logger::LogHeader(const G4String& title)
{
    if (!fEnabled) return;
    
    LogLine("");
    LogSeparator('=');
    LogLine("  " + title);
    LogSeparator('=');
}
