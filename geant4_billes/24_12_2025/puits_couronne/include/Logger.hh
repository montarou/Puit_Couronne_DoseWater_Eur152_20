#ifndef Logger_h
#define Logger_h 1

#include "globals.hh"
#include <fstream>
#include <string>

/// @brief Système de logging pour rediriger les diagnostics vers un fichier
///
/// Singleton qui gère l'écriture des messages de diagnostic dans output.log
/// Utilisation: Logger::GetInstance()->Log("message");

class Logger
{
public:
    /// Retourne l'instance unique du Logger
    static Logger* GetInstance();
    
    /// Ouvre le fichier de log
    void Open(const G4String& filename = "output.log");
    
    /// Ferme le fichier de log
    void Close();
    
    /// Écrit un message dans le log
    void Log(const G4String& message);
    
    /// Écrit un message avec retour à la ligne
    void LogLine(const G4String& message);
    
    /// Écrit une ligne de séparation
    void LogSeparator(char c = '=', int length = 70);
    
    /// Écrit un header encadré
    void LogHeader(const G4String& title);
    
    /// Retourne le stream pour écriture directe
    std::ofstream& GetStream() { return fLogFile; }
    
    /// Vérifie si le fichier est ouvert
    G4bool IsOpen() const { return fLogFile.is_open(); }
    
    /// Active/désactive le logging
    void SetEnabled(G4bool enabled) { fEnabled = enabled; }
    G4bool IsEnabled() const { return fEnabled; }
    
    /// Active/désactive l'écho sur la console
    void SetEchoToConsole(G4bool echo) { fEchoToConsole = echo; }
    G4bool GetEchoToConsole() const { return fEchoToConsole; }

private:
    Logger();
    ~Logger();
    
    // Empêcher la copie
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    static Logger* fInstance;
    std::ofstream fLogFile;
    G4bool fEnabled;
    G4bool fEchoToConsole;
    G4String fFilename;
};

// Macro pour simplifier l'utilisation
#define LOG(msg) Logger::GetInstance()->LogLine(msg)
#define LOG_STREAM Logger::GetInstance()->GetStream()

#endif
