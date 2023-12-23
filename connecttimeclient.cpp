#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <filesystem>
#include <net/if.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <cmath>
#include <fstream>


#define DEF_PORT 23365
#define DEF_BUFFER_SIZE 1024
#define DEF_POS_IPS 254

using namespace std;

string wrld_S_clientName;
int wrld_arr_i_IP3[DEF_POS_IPS];
mutex wrld_arr_mutex;

std::atomic<int> total_progress(0);
const int total_addresses = DEF_POS_IPS * DEF_POS_IPS;

bool getEth0InetAddress(string& pointer_S_ip_address)
{
    cout << "Avvio funzione getEth0InetAddress" << endl;
    int sock;
    struct ifreq strc_ifreq;
    strc_ifreq.ifr_addr.sa_family = AF_INET;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        cerr << "Err: Impossibile creare il socket N. " << -(101) << endl;
        return false;
    }

    strncpy(strc_ifreq.ifr_name, "eth0", IFNAMSIZ-1);

    if (ioctl(sock, SIOCGIFADDR, &strc_ifreq) == -1) 
    {
        cerr << "Err: Impossibile ottenere l'indirizzo IP per eth0 N. " << -(102) << endl;
        close(sock);
        return false;
    }

    close(sock);

    auto* ip_addr = (struct sockaddr_in*)&strc_ifreq.ifr_addr;
    string full_ip = inet_ntoa(ip_addr->sin_addr);

    size_t second_dot = full_ip.find('.', full_ip.find('.') + 1);

    if (second_dot != string::npos) 
    {
        pointer_S_ip_address = full_ip.substr(0, second_dot + 1);
        return true;
    }

    return false;
}

string S_ip_ext;
bool Eth0ret = getEth0InetAddress(S_ip_ext);

#define DEF_IP S_ip_ext;

void printProgressBar()
{
    cout << "Avvio funzione printProgressBar" << endl;
    while (total_progress < total_addresses) 
    {
        int progress = total_progress.load();
        int percentage = int(progress * 100.0 / total_addresses);
        std::cout << percentage << " %" << "\r";
        std::cout.flush();
        this_thread::sleep_for(chrono::milliseconds(100));  // Update every 100 milliseconds
    }
    std::cout << endl; // Add a newline at the end for better formatting
}

void scanBlock(int start, int end, int& i_C)
{
    cout << "Avvio funzione scanBlock" << endl;
    for (int i = start; i <= end; ++i)
    {
        for (int j = 0; j <= 255; ++j)
        {
            string ip = DEF_IP
            string ipDP = ip + to_string(i) + "." + to_string(j);

            string command = "nmap -sn " + ipDP + " --host-timeout 700ms > /dev/null 2>&1";

            if (system(command.c_str()) == 0) 
            {
                lock_guard<mutex> lock(wrld_arr_mutex);
                wrld_arr_i_IP3[i_C] = i;
                i_C++;
            }

            total_progress++;
        }
    }
}

void scanLocalNetwork() 
{
    cout << "Avvio funzione scanLocalNetwork" << endl;
    int numThreads = floor(thread::hardware_concurrency() / 3 * 2);
    if (numThreads < 1) 
    {
        numThreads = 1;
    }

    int block_size = DEF_POS_IPS / numThreads;
    vector<thread> threads;
    int i_C = 0;

    // Avvia i thread per la scansione
    for (int i = 0; i < numThreads; ++i) 
    {
        int start = i * block_size;
        int end = (i + 1) * block_size - 1;
        threads.emplace_back(scanBlock, start, end, std::ref(i_C));
    }

    // Avvia un thread separato per la barra di progresso
    thread progress_thread(printProgressBar);

    // Attendi il completamento di tutti i thread di scansione
    for (auto &th : threads) 
    {
        th.join();
    }

    // Assicurati che la barra di progresso sia completa
    total_progress = total_addresses;
    progress_thread.join();

    std::cout << endl;  // Vai a capo alla fine del processo
}

int CreateSocket()
{
    std::cout << "Avvio funzione CreateSocket" << endl;
    int i_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (i_sock < 0) 
    {
        cerr << "Err: Impossibile creare il socket N. " << -(103) << endl;
        return -(103);
    }

    struct timeval strc_tv;
    strc_tv.tv_sec = 10;
    strc_tv.tv_usec = 0;

    setsockopt(i_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&strc_tv, sizeof strc_tv);
    setsockopt(i_sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&strc_tv, sizeof strc_tv);

    return i_sock;
}

bool ConnectToServer(int i_sock, const string &const_S_ip) 
{
    std::cout << "Avvio funzione ConnectToServer" << endl;

    // Impostazione del timeout
    struct timeval timeout;
    timeout.tv_sec = 0;  // Secondi
    timeout.tv_usec = 70;  // 500000 microsecondi = 0.5 secondi

    if (setsockopt(i_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) 
    {
        cerr << "Err: Impossibile impostare il timeout di ricezione N. " << -(110) << endl;
        return false;
    }

    if (setsockopt(i_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) 
    {
        cerr << "Err: Impossibile impostare il timeout di invio N. " << -(111) << endl;
        return false;
    }

    struct sockaddr_in strc_server;
    strc_server.sin_family = AF_INET;
    strc_server.sin_addr.s_addr = inet_addr(const_S_ip.c_str());
    strc_server.sin_port = htons(DEF_PORT);

    if (connect(i_sock, (struct sockaddr*)&strc_server, sizeof(strc_server)) < 0) 
    {
        cerr << "Err: Connessione al server fallita N. " << -(104) << endl;
        return false;
    }

    return true;
}

bool SendMessageToServer(int i_sock, const string &S_message) 
{
    std::cout << "Avvio funzione SendMessageToServer" << endl;
    if (send(i_sock, S_message.c_str(), DEF_BUFFER_SIZE, 0) < 0) 
    {
        cerr << "Err: Invio del messaggio fallito N. " << -(105) << endl;
        return false;
    }
    return true;
}

bool IsPortOpen(const string &const_S_ip, int i_port, char **pointer_pointer_c_nome) 
{
    std::cout << "Avvio funzione IsPortOpen" << endl;
    char arr_c_buffer[DEF_BUFFER_SIZE];
    int i_sock = CreateSocket();
    if (i_sock < 0) return false;

    if (!ConnectToServer(i_sock, const_S_ip)) 
    {
        close(i_sock);
        cerr << "Err: Connessione al server fallita N. " << -(106) << endl;
        return false;
    }

    if (!SendMessageToServer(i_sock, "NAME")) 
    {
        close(i_sock);
        cerr << "Err: Invio del nome fallito N. " << -(107) << endl;
        return false;
    }

    if (recv(i_sock, (void *)arr_c_buffer, sizeof(arr_c_buffer), 0) < 0) 
    {
        close(i_sock);
        cerr << "Err: Ricezione del nome fallita N. " << -(108) << endl;
        return false;
    }

    *pointer_pointer_c_nome = arr_c_buffer;

    close(i_sock);
    return true;
}

bool Ricerca()
{
    std::cout << "Avvio funzione Ricerca" << endl;
    char *pointer_c_nome;
    bool b_ForNF = false;
    string S_networkBase = DEF_IP;
    bool b_StatePort;
    string S_ip;

    std::cout << "Lista server:\n\r";

    for (int i = 0; i <= 255; i++)
    {
        if(wrld_arr_i_IP3[i] != -1)
        {
            for (int j = 0; j < 255; j++) 
            {
                
                S_ip = S_networkBase + to_string(wrld_arr_i_IP3[i]) + "." + to_string(j);

                std::cout << "Provo ip: " << S_ip << endl;

                b_StatePort = IsPortOpen(S_ip, DEF_PORT, &pointer_c_nome);

                if (b_StatePort) 
                {
                    std::cout << "L'ip di " << pointer_c_nome << " è " << S_ip << endl;
                    b_ForNF = true;
                    return b_ForNF;
                }
            }
        }
        else
        {
            break;
        }
    }
    return b_ForNF;
}

bool GetServerIP(string &S_ip)
{
    std::cout << "Avvio funzione GetServerIP" << endl;
    std::cout << "a chi ti vuoi connettere?" << endl;
    cin >> S_ip;

    return !S_ip.empty();
}

void ChatWithOne() 
{
    std::cout << "Avvio funzione ChatWithOne" << endl;
    string S_ip;
    if (!Ricerca() || !GetServerIP(S_ip)) 
    {
        cerr << "Errore nella ricerca dell'ip o IP non fornito.\n";
        return;
    }

    int i_sock = CreateSocket();
    if (i_sock < 0) return;

    if (!ConnectToServer(i_sock, S_ip)) 
    {
        cerr << "Connessione fallita\n";
        close(i_sock);
        return;
    }

    std::cout << "scrivi stopmess per fermare i messaggi" << endl;
    string S_comando;
    while (true) 
    {
        getline(cin >> ws, S_comando);
        if (S_comando == "stopmess") break;

        string mess = "MESS-" + wrld_S_clientName + "> " + S_comando;
        SendMessageToServer(i_sock, mess);
    }
    close(i_sock);
}

void BroadCast(const string &S_messaggio) 
{
    std::cout << "Avvio funzione BroadCast" << endl;
    char *pointer_c_nome;
    string S_networkBase = DEF_IP;
    string S_ip;
    bool b_StatePort;

    for (int i = 0; i < 255; ++i) 
    {
        for (int j = 0; j < 255; j++) 
    {
            S_ip = S_networkBase + to_string(i) + "." + to_string(j);

            b_StatePort = IsPortOpen(S_ip, DEF_PORT, &pointer_c_nome);

            if (b_StatePort) 
    {
                int i_sock = CreateSocket();
                if (i_sock < 0) continue;

                if (ConnectToServer(i_sock, S_ip)) 
    {
                    string S_messaggiocom = "MESS-" + wrld_S_clientName + "> " + S_messaggio;
                    SendMessageToServer(i_sock, S_messaggiocom);
                }
                close(i_sock);
            }
        }
    }
}

string getLinuxDistro() 
{
    cout << "Avvio funzione getLinuxDistro" << endl;
    std::string line;
    std::ifstream infile("/etc/os-release");
    while (std::getline(infile, line)) 
    {
        // Cerca la linea che specifica l'ID della distribuzione
        if (line.rfind("ID=", 0) == 0) 
    { // assicurati che la linea inizi con "ID="
            // Estrai il valore dopo "ID="
            std::string distroId = line.substr(3);

            // Rimuovi le virgolette se presenti
            distroId.erase(std::remove(distroId.begin(), distroId.end(), '\"'), distroId.end());

            return distroId;
        }
    }
    return "unknown";
}

void installNmap() 
{
    cout << "Avvio funzione installNmap" << endl;
    std::string distro = getLinuxDistro();
    
    if (distro == "ubuntu" || distro == "debian") 
    {
        system("sudo apt-get install -y nmap");
    } else if (distro == "fedora" || distro == "centos" || distro == "rhel") 
    {
        system("sudo dnf install -y nmap");
    } else if (distro == "arch" || distro == "manjaro") 
    {
        system("sudo pacman -S nmap");
    } else if (distro == "opensuse" || distro == "sles") 
    {
        system("sudo zypper install -y nmap");
    } else {
        std::cout << "Distribuzione non supportata o sconosciuta: " << distro << std::endl;
    }
}

int main() 
{
    try 
    {
        installNmap();

        for (int i = 0; i < 254; ++i) 
        {
            wrld_arr_i_IP3[i] = -1;
        }
        
        scanLocalNetwork();

        for (int i = 0; i < 255; ++i) 
        {
            std::cout << "Elemento " << i << ": " << wrld_arr_i_IP3[i] << endl;
        }

        if (!Eth0ret) 
        {
            std::cout << "Err: Errore nella connect N. " << -(109) << endl;
            return -(109);
        }

        string S_comando;
        std::cout << "Scrivi il tuo nome: ";
        std::cin >> wrld_S_clientName;
        std::cout << endl;

        char c_switch;
        while (true) 
        {
            std::cout << "che vuoi fare?" << endl
                << "1: chatta con uno" << endl
                << "2: chatta con tutti" << endl
                << "3: manda file" << endl
                << "4: basta" << endl;
            cin >> c_switch;

            switch(c_switch) 
            {
                case '1':
                    ChatWithOne();
                    break;
                case '2':
                    std::cout << "scrivi stopmess per fermare i messaggi" << endl;
                    
                    while (true) 
                    {
                        getline(cin >> ws, S_comando);
                        if (S_comando == "stopmess") break;
                        BroadCast(S_comando);
                    }
                    break;
                case '3':
                    /// Mancante
                    break;
                case '4':
                    return 0;
                default:
                    std::cout << "Comando non valido." << endl;
            }
        }
        return 0;
    } catch (const std::system_error& e) 
    {
        cerr << "System error: " << e.what() << endl;
    } catch (const std::exception& e) 
    {
        cerr << "Exception: " << e.what() << endl;
    } catch (...) 
    {
        cerr << "An unknown error occurred." << endl;
    }
    return 0;
}