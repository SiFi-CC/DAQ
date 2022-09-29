#include <zmq.hpp>
#include "nlohmann/json.hpp"
#include <TROOT.h>
#include <TSystem.h>
#include <TH1.h>
#include <TH2.h>
#include <THttpServer.h>
#include <TCanvas.h>
#include <fstream>
void tokenize(std::string str, std::vector<std::string> &token_v, char delimiter = '/'){
    size_t start = str.find_first_not_of(delimiter), end=start;
    while (start != std::string::npos) {
        // Find next occurence of delimiter
        end = str.find(delimiter, start);
        // Push back the token found into vector
        token_v.push_back(str.substr(start, end-start));
        // Skip all occurences of the delimiter to find new start
        start = str.find_first_not_of(delimiter, end);
    }
}
void findFiberAddress(const char * f, std::vector<std::map<std::string, std::string> > &address, const char *section = "FibersPMILookupTable") {
    std::ifstream file(f);
    std::string line;
    Bool_t start = kFALSE;
    while(getline(file, line) ) {
        std::string sectionName = "["+std::string(section)+"]";
        if (line.find(sectionName) != std::string::npos) {
            getline(file, line);
            getline(file, line);
            start = kTRUE;
        }
        if(start) {
            std::string::size_type pos;
            std::vector<std::string> token;
            while((pos = line.find("\t") ) != std::string::npos) {
                line.replace(pos, 1, " ");
            }
            while((pos = line.find("  ") ) != std::string::npos) {
                line.replace(pos, 2, " ");
            }
            tokenize(line, token, ' ');
            std::map<std::string, std::string> mapAddr;
            mapAddr["m"] = token[2];
            mapAddr["l"] = token[3];
            mapAddr["f"] = token[4];
            mapAddr["s"] = token[5];
            //ignore channels from external trigger (4096, 4097)
            if(std::stoi(token[1]) > 320) continue;
            address[std::stoi(token[1]) ] = mapAddr;
        }
    }
}
int main(int argc, char *argv[]) {
    //channel mapping
    std::vector<std::map<std::string, std::string> > address;
    std::vector<TH1F *> vecQDC;
    gDirectory->mkdir("energy");
    gDirectory->cd("/energy");
    for(UInt_t i=0; i < 320; ++i) {
        std::map<std::string, std::string> m = {{"m",""}, {"l",""}, {"f",""}, {"s",""} };
        address.push_back(m);
        //create vector of histograms to fill qdc
        vecQDC.push_back(new TH1F(Form("hQDC%d", i), Form("%d;qdc", i), 10, 0, 100) ); //qdc 
    }
    findFiberAddress("sifi_params.txt", address);
    gDirectory->cd("/");
    gDirectory->mkdir("hits");
    gDirectory->cd("/hits");
    TH2I *hHitsL = new TH2I("hHitsL", "L;fiber;layer", 16, 0, 16, 4, 0, 4);
    TH2I *hHitsR = new TH2I("hHitsR", "R;fiber;layer", 16, 0, 16, 4, 0, 4);
    
    zmq::context_t ctx;
    zmq::socket_t sub(ctx, ZMQ_SUB);
    //connect to the zmq publisher created in the daq machine
    sub.connect("tcp://172.16.32.214:2000");
    sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    
    //start the monitoring server locally
    const char *url = "http:127.0.0.1:8888";
    THttpServer *server = new THttpServer(url);
    TCanvas *canQDCR = new TCanvas("canQDCR", "canQDCR");
    canQDCR->Divide(16, 4);
    for(UInt_t i=0; i < 16; ++i) {
        for(UInt_t j=0; j < 4; ++j) {
            canQDCR->cd(1+ i + 16*j);
            vecQDC[i + 16*j]->Draw();
        }
    }
    TCanvas *canQDCL = new TCanvas("canQDCL", "canQDCL");
    canQDCL->Divide(16, 4);
    for(UInt_t i=0; i < 16; ++i) {
        for(UInt_t j=0; j < 4; ++j) {
            canQDCL->cd(1+ i + 16*j);
            vecQDC[256 + i + 16*j]->Draw();
        }
    }
    Double_t Tps = 1E12 / 200.e6;
    Float_t Tns = Tps / 1.e3;
    while(true) {
        zmq::message_t msg;
        try {
            //message from the zmq publisher
            sub.recv(msg);
        } catch(zmq::error_t &e) {
            fprintf(stderr, "%s\n", e.what() );
        }
        //parsing string object into json object
        std::string readBuffer = msg.to_string();
        nlohmann::json j;
        try {
            j = nlohmann::json::parse(readBuffer);
        } catch(nlohmann::json::parse_error &e) {
            fprintf(stderr, "%d %s\n", e.id, e.what() );
        }
        nlohmann::json ev = j["events"];
        for(UShort_t x=0; x < ev.size(); ++x) {
            int channelID = ev[x][0].get<int>();
            vecQDC[channelID]->Fill(ev[x][5].get<int>() / Tns ); //EFine
            if(address[channelID]["s"].compare("l") == 0) {
                //fill hits on the left
                hHitsL->Fill(std::stoi(address[channelID]["f"]), std::stoi(address[channelID]["l"]) );
            }
            if(address[channelID]["s"].compare("r") == 0) {
                //fill hits on the right
                hHitsR->Fill(std::stoi(address[channelID]["f"]), std::stoi(address[channelID]["l"]) );
            }
        }
        if (gSystem->ProcessEvents()) break;
    }
    return 0;
}
