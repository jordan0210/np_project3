#include "cgi_server.h"

class session
    : public std::enable_shared_from_this<session>{
    public:
        session(tcp::socket socket)
            : socket_(std::move(socket)){
            }

        void start(){
            do_read();
        }

    private:
        void do_read(){
            auto self(shared_from_this());
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                [this, self](boost::system::error_code ec, std::size_t length){

                if (!ec){
                    string HttpRequest = data_;

                    parseHttpRequest(HttpRequest);

                    setHttpEnv();
                    setenv("REMOTE_ADDR", socket_.remote_endpoint().address().to_string().c_str(), 1);
                    setenv("REMOTE_PORT", to_string(socket_.remote_endpoint().port()).c_str(), 1);
                    dup2(socket_.native_handle(), STDOUT_FILENO);
                    cout << "HTTP/1.1 200 OK\r\n";
                    cout.flush();

                    string URI = getenv("REQUEST_URI");
                    if (URI == "/panel.cgi"){
                        panelCgi();
                    } else if (URI == "/console.cgi"){
                        consoleCgi();
                    }
                }
            });
        }

        void do_write(std::size_t length){
            auto self(shared_from_this());
            boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
                [this, self](boost::system::error_code ec, std::size_t /*length*/){

                if (!ec){
                    do_read();
                }
            });
        }

        tcp::socket socket_;
        enum { max_length = 1024 };
        char data_[max_length];
};

class server{
    public:
        server(boost::asio::io_context& io_context, short port)
            : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
            do_accept();
        }

    private:
        void do_accept(){
            acceptor_.async_accept(
                [this](boost::system::error_code ec, tcp::socket socket){
                if (!ec){
                    std::make_shared<session>(std::move(socket))->start();
                }

                do_accept();
            });
        }

        tcp::acceptor acceptor_;
};

class client
    : public std::enable_shared_from_this<client>{
    public:
        client()
            : io_context_(io_context), socket_(io_context), ID_(ID){

            do_connect(endpoints);
        }

        // void close(){
        //     boost::asio::post(io_context_, [this]() { socket_.close(); });
        // }

    private:
        void do_connect(const tcp::resolver::results_type& endpoints){
            auto self(shared_from_this());
            boost::asio::async_connect(socket_, endpoints,
                [this, self](boost::system::error_code ec, tcp::endpoint){

                if (!ec){
                    // bzero(data_, sizeof(data_));
                    cerr << "connect success.\n";
                    string path = "./test_case/" + requestDatas[stoi(ID_)].testfile;
                    int fd_testfile = open(path.data(), O_RDONLY);
                    cerr << "file open success.\n";
                    dup2(fd_testfile, STDIN_FILENO);
                    close(fd_testfile);
                    do_read();
                }
            });
        }

        void do_read(){
            auto self(shared_from_this());
            cerr << "Start reading.\n";
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
                [this, self](boost::system::error_code ec, std::size_t length){
                cerr << "Success reading.\n";
                if (!ec){
                    // cerr << "Success reading.\n";
                    data_[length] = '\0';
                    string Msg = data_;
                    // bzero(data_, sizeof(data_));

                    send_shell(ID_, Msg);
                    if (length != 0){
                        if ((int)Msg.find('%', 0) < 0){
                            do_read();
                        } else {
                            string command;
                            getline(cin, command);
                            command += "\n";
                            send_command(ID_, command);
                            do_write(command);
                        }
                    }
                } else {
                    socket_.close();
                }
            });
            cerr << "End reading.\n";
        }

        void do_write(string origin_Msg){
            auto self(shared_from_this());
            const char *Msg = origin_Msg.c_str();
            char unit;
            boost::asio::async_write(socket_, boost::asio::buffer(Msg, sizeof(unit)*origin_Msg.length()),
                [this, self, &origin_Msg](boost::system::error_code ec, std::size_t /*length*/){

                if (!ec){
                    if (origin_Msg != "exit\n"){
                        do_read();
                    } else {
                        socket_.close();
                    }
                }
            });
        }

        boost::asio::io_context& io_context_;
        tcp::socket socket_;
        string ID_;
        enum { max_length = 1024 };
        char data_[max_length];
};

int main(int argc, char* argv[]){
    test_argv = argv;
    try{
        if (argc != 2){
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        server s(io_context, std::atoi(argv[1]));

        io_context.run();
    } catch (std::exception& e){
        std::cerr << "Exception: " << e.what() << "\n";
    }

  return 0;
}

void panelCgi(){
    cout << "Content-type: text/html\r\n\r\n" << flush;

    string host_menu = "";
    for (int i=1; i<13; i++){
        host_menu = host_menu + "<option value=\"nplinux" + to_string(i) + ".cs.nctu.edu.tw\">nplinux" + to_string(i) + "</option>";
    }

    string test_case_menu = "";
    for (int i=1; i<11; i++){
        test_case_menu = test_case_menu + "<option value=\"t" + to_string(i) + ".txt\">t" + to_string(i) + ".txt</option>";
    }

    cout << "\
    <!DOCTYPE html>\
    <html lang=\"en\">\
        <head>\
            <title>NP Project 3 Panel</title>\
            <link\
                rel=\"stylesheet\"\
                href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\
                integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\
                crossorigin=\"anonymous\"\
            />\
            <link\
                href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\
                rel=\"stylesheet\"\
            />\
            <link\
                rel=\"icon\"\
                type=\"image/png\"\
                href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\
            />\
            <style>\
                * {\
                    font-family: 'Source Code Pro', monospace;\
                }\
            </style>\
        </head>\
        <body class=\"bg-secondary pt-5\">\
            <form action=\"console.cgi\" method=\"GET\">\
                <table class=\"table mx-auto bg-light\" style=\"width: inherit\">\
                    <thead class=\"thead-dark\">\
                        <tr>\
                            <th scope=\"col\">#</th>\
                            <th scope=\"col\">Host</th>\
                            <th scope=\"col\">Port</th>\
                            <th scope=\"col\">Input File</th>\
                        </tr>\
                    </thead>\
                    <tbody>" << flush;

    for (int i=0; i<5; i++){
        cout << "\
                        <tr>\
                            <th scope=\"row\" class=\"align-middle\">Session " << i + 1 << "</th>\
                            <td>\
                                <div class=\"input-group\">\
                                    <select name=\"h" << i << "\" class=\"custom-select\">\
                                        <option></option>" << host_menu << "\
                                    </select>\
                                    <div class=\"input-group-append\">\
                                        <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\
                                    </div>\
                                </div>\
                            </td>\
                            <td>\
                                <input name=\"p" << i << "\" type=\"text\" class=\"form-control\" size=\"5\" />\
                            </td>\
                            <td>\
                                <select name=\"f" << i << "\" class=\"custom-select\">\
                                    <option></option>" << test_case_menu << "\
                                </select>\
                            </td>\
                        </tr>" << flush;
    }

    cout << "\
                        <tr>\
                            <td colspan=\"3\"></td>\
                            <td>\
                                <button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\
                            </td>\
                        </tr>\
                    </tbody>\
                </table>\
            </form>\
        </body>\
    </html>" << flush;
}

int consoleCgi(){
    // bzero(requestDatas, sizeof(requestDatas));
    string QUERY_STRING = getenv("QUERY_STRING");
    parse_QUERY_STRING(QUERY_STRING);
    send_default_HTML();

    try{
        vector<clientData> clientDatas;
        vector<shared_ptr<client>> clients;
        vector<thread> threads;
        for (int i=0; i<5; i++){
            if (requestDatas[i].url.length() == 0)
                break;
            send_dafault_table(to_string(i), (requestDatas[i].url + ":" + requestDatas[i].port));

            clientData newClientData;
            tcp::resolver resolver(io_contexts[i]);
            newClientData.endpoints = resolver.resolve(requestDatas[i].url.data(), requestDatas[i].port.data());
            newClientData.ID = to_string(i);
            newClientData.testfile = requestDatas[i].testfile;
            clientDatas.push_back(newClientData);
        }
        clientData *cD = clientDatas;
        for (int i=0; i<clientDatas.size(); i++){

            clients.emplace_back(new client(cd));

        }
        //     // boost::asio::io_context io_context;
        //     tcp::resolver resolver(io_contexts[i]);
        //     auto endpoints = resolver.resolve(requestDatas[i].url.data(), requestDatas[i].port.data());
        //     // client c(io_contexts[i], endpoints, to_string(i));
        //     IDs[i] = to_string(i);
        //     clients.emplace_back(io_contexts[i], endpoints, IDs[i]);
        //     threads.emplace_back([&io_contexts, i](){
        //         cerr << "thread " << i << " start.\n";
        //         io_contexts[i].run();
        //     });
        // }
        // for(int i=0; i<2; i++){
        //     threads[i].join();
        // }
    } catch (exception& e){
        cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}

// -------http_server-------
void parseHttpRequest(string HttpRequest){
    int firstLineIndex = HttpRequest.find('\n', 0);
    int secondLineIndex = HttpRequest.find('\n', firstLineIndex + 1);
    string Line1 = HttpRequest.substr(0, firstLineIndex-1);
    string Line2 = HttpRequest.substr(firstLineIndex+1, secondLineIndex - firstLineIndex - 2);

    int startIndex, endIndex;
    startIndex = 0;
    endIndex = Line1.find(' ', 0);
    envVars.values[0] = Line1.substr(startIndex, endIndex - startIndex);

    startIndex = endIndex + 1;
    if ((endIndex = Line1.find('?', startIndex)) != -1){
        endIndex = Line1.find('?', startIndex);
        envVars.values[1] = Line1.substr(startIndex, endIndex - startIndex);

        startIndex = endIndex + 1;
        endIndex = Line1.find(' ', startIndex);
        envVars.values[2] = Line1.substr(startIndex, endIndex - startIndex);
    } else {
        endIndex = Line1.find(' ', startIndex);
        envVars.values[1] = Line1.substr(startIndex, endIndex - startIndex);

        envVars.values[2] = "";
    }

    startIndex = endIndex + 1;
    envVars.values[3] = Line1.substr(startIndex);

    Line2 = Line2.substr(Line2.find(' ', 0) + 1);
    envVars.values[4] = Line2;
    envVars.values[5] = Line2.substr(0, Line2.find(':', 0));
    envVars.values[6] = Line2.substr(Line2.find(':', 0) + 1);
}

void setHttpEnv(){
    for (int i=0; i<7; i++){
        setenv(envVars.names[i].data(), envVars.values[i].data(), 1);
    }
}
// -------------------------

// -------console.cgi-------
void parse_QUERY_STRING(string &QUERY_STRING){
    QUERY_STRING = QUERY_STRING + "&";
    string requestBlock;
    int start, end, index;
    start = 0;
    index = 0;
    while ((end = QUERY_STRING.find('&', start)) != -1){
        requestBlock = QUERY_STRING.substr(start, end-start);
        if (requestBlock.length() == 3)
            break;
        if (index % 3 == 0){
            requestDatas[index/3].url = requestBlock.substr(3);
        } else if (index % 3 == 1){
            requestDatas[index/3].port = requestBlock.substr(3);
        } else {
            requestDatas[index/3].testfile = requestBlock.substr(3);
        }
        index++;
        start = end + 1;
    }
}

void send_default_HTML(){
    cout << "Content-type: text/html\r\n\r\n";
    cout << "\
    <!DOCTYPE html>\
    <html lang=\"en\">\
        <head>\
            <meta charset=\"UTF-8\" />\
            <title>NP Project 3 Sample Console</title>\
            <link\
                rel=\"stylesheet\"\
                href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\"\
                integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\"\
                crossorigin=\"anonymous\"\
            />\
            <link\
                href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\
                rel=\"stylesheet\"\
            />\
            <link\
                rel=\"icon\"\
                type=\"image/png\"\
                href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\
            />\
            <style>\
            * {\
                font-family: 'Source Code Pro', monospace;\
                font-size: 1rem !important;\
            }\
            body {\
                background-color: #212529;\
            }\
            pre {\
                color: #cccccc;\
            }\
            b {\
                color: #01b468;\
            }\
            </style>\
        </head>\
        <body>\
            <table class=\"table table-dark table-bordered\">\
                <thead>\
                    <tr id=\"tableHead\">\
                    </tr>\
                </thead>\
                <tbody>\
                    <tr id=\"tableBody\">\
                    </tr>\
                </tbody>\
            </table>\
        </body>\
    </html>";
    cout.flush();
}

void send_dafault_table(string index, string Msg){
    Msg = "<th scope=\\\"col\\\">" + Msg + "</th>";
    cout << "<script>document.getElementById('tableHead').innerHTML += '" << Msg << "';</script>";
    cout.flush();
    Msg = "<td><pre id=\\\"s" + index + "\\\" class=\\\"mb-0\\\"></pre></td>";
    cout << "<script>document.getElementById('tableBody').innerHTML += '" << Msg << "';</script>";
    cout.flush();
}

void send_command(string index, string Msg){
    // cmdCount++;
    // Msg = to_string(cmdCount) + " : " + Msg;
    refactor(Msg);
    cout << "<script>document.getElementById('s" + index + "').innerHTML += '<b>" << Msg << "</b>';</script>";
    cout.flush();
}

void send_shell(string index, string Msg){
    refactor(Msg);
    cout << "<script>document.getElementById('s" + index + "').innerHTML += '" << Msg << "';</script>";
    cout.flush();
}

void refactor(string &Msg){
    string returnMsg = "";
    for (int i=0; i<(int)Msg.length(); i++){
        if (Msg[i] == '\n'){
            returnMsg += "<br>";
        } else if (Msg[i] == '\r'){
            returnMsg += "";
        } else if (Msg[i] == '\''){
            returnMsg += "\\'";
        } else if (Msg[i] == '<'){
            returnMsg += "&lt;";
        } else if (Msg[i] == '>'){
            returnMsg += "&gt;";
        }

        else {
            returnMsg += Msg[i];
        }
    }
    Msg = move(returnMsg);
}
// -------------------------