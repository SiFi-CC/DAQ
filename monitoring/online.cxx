#include <zmq.hpp>
#include "nlohmann/json.hpp"
#include <TROOT.h>
#include <TSystem.h>
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <THttpServer.h>
#include "TRootSniffer.h"
#include <TCanvas.h>
#include <fstream>

#include "SControl.h"
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
void findFiberAddress(const char * f, std::vector<std::map<std::string, std::string> > &address, const char *section = "FibersTPLookupTable") {
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
    TFile *fOutput = new TFile("monitoring.root", "RECREATE");
    // frames and errors
    gDirectory->mkdir("frames");
    gDirectory->cd("/frames");
    TH1I *hFrames = new TH1I("hFrames", ";Frame ID", 100, 0, 100E7);
    TH1I *hFramesLost = new TH1I("hFramesLost", ";Frame ID", 100, 0, 100E7);
    // channel mapping
    std::vector<std::map<std::string, std::string> > address;
    std::vector<TH1F *> vecQDC;
    std::vector<TH1F *> vecTime;
    gDirectory->cd("/");
    gDirectory->mkdir("events");
    gDirectory->cd("/events");
    for(UInt_t i=0; i < 320; ++i) {
        std::map<std::string, std::string> m = {{"m",""}, {"l",""}, {"f",""}, {"s",""} };
        address.push_back(m);
        //create vector of histograms to fill qdc and time
        vecQDC.push_back(new TH1F(Form("hQDC%d", i), Form("%d;qdc", i), 10, 0, 100) ); //qdc 
        vecTime.push_back(new TH1F(Form("hTime%d", i), Form("%d;t[ns]", i), 100, 0, 10000) ); //time 
    }
    findFiberAddress("../sandbox/sifi_params.txt", address);
    gDirectory->cd("/");
    gDirectory->mkdir("hits");
    gDirectory->cd("/hits");
    TH2S *hHitsL = new TH2S("hHitsL", "L;fiber;layer", 16, 0, 16, 4, 0, 4);
    TH2S *hHitsR = new TH2S("hHitsR", "R;fiber;layer", 16, 0, 16, 4, 0, 4);
    hHitsL->SetOption("COLZ");
    hHitsR->SetOption("COLZ");
    
    gDirectory->cd("/");
    gDirectory->mkdir("rshmp4040");
    gDirectory->cd("/rshmp4040");
    TGraph *gCh3_c = new TGraph();
    gCh3_c->SetName("gCh3_c");
    gCh3_c->SetTitle("Ch3 current; ;I[A]");
    gCh3_c->GetXaxis()->SetTimeDisplay(1);
    gCh3_c->GetXaxis()->SetTimeFormat("#splitline{%b %d}{%H:%M:%S}");
    gCh3_c->GetYaxis()->SetMaxDigits(3);
    gDirectory->Add(gCh3_c);
    TGraph *gCh3_v = new TGraph();
    gCh3_v->SetName("gCh3_v");
    gCh3_v->SetTitle("Ch3 voltage; ;V[V]");
    gCh3_v->GetXaxis()->SetTimeDisplay(1);
    gCh3_v->GetXaxis()->SetTimeFormat("#splitline{%b %d}{%H:%M:%S}");
    gCh3_v->GetYaxis()->SetMaxDigits(3);
    gDirectory->Add(gCh3_v);
    TGraph *gCh4_c = new TGraph();
    gCh4_c->SetName("gCh4_c");
    gCh4_c->SetTitle("Ch4 current; ;I[A]");
    gCh4_c->GetXaxis()->SetTimeDisplay(1);
    gCh4_c->GetXaxis()->SetTimeFormat("#splitline{%b %d}{%H:%M:%S}");
    gCh4_c->GetYaxis()->SetMaxDigits(3);
    gDirectory->Add(gCh4_c);
    TGraph *gCh4_v = new TGraph();
    gCh4_v->SetName("gCh4_v");
    gCh4_v->SetTitle("Ch4 voltage; ;V[V]");
    gCh4_v->GetXaxis()->SetTimeDisplay(1);
    gCh4_v->GetXaxis()->SetTimeFormat("#splitline{%b %d}{%H:%M:%S}");
    gCh4_v->GetYaxis()->SetMaxDigits(3);
    gDirectory->Add(gCh4_v);

    //connect to the zmq publisher created in the daq machine
    zmq::context_t ctx;
    zmq::socket_t sub(ctx, ZMQ_SUB);
    sub.connect("tcp://172.16.32.214:2000");
    sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    //connect to the zmq publisher created in the devices
    zmq::context_t ctx0;
    zmq::socket_t sub0(ctx0, ZMQ_SUB);
    sub0.connect("tcp://172.16.32.214:2001");
    sub0.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    
    //start the monitoring server locally
    // const char *url = "http:127.0.0.1:8888";
    const char *url = "http:172.16.32.214:8888";
    THttpServer *server = new THttpServer(url);
    server->RegisterCommand("/Start", "SControl::Start()", "button;rootsys/icons/ed_execute.png");
    server->RegisterCommand("/Stop",  "SControl::Stop()", "button;rootsys/icons/ed_interrupt.png");
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
    TCanvas *canTimeR = new TCanvas("canTimeR", "canTimeR");
    canTimeR->Divide(16, 4);
    for(UInt_t i=0; i < 16; ++i) {
        for(UInt_t j=0; j < 4; ++j) {
            canTimeR->cd(1+ i + 16*j);
            vecTime[i + 16*j]->Draw();
        }
    }
    TCanvas *canTimeL = new TCanvas("canTimeL", "canTimeL");
    canTimeL->Divide(16, 4);
    for(UInt_t i=0; i < 16; ++i) {
        for(UInt_t j=0; j < 4; ++j) {
            canTimeL->cd(1+ i + 16*j);
            vecTime[256 + i + 16*j]->Draw();
        }
    }
    TCanvas *canSlowControl = new TCanvas("canSlowControl", "canSloControl");
    canSlowControl->Divide(2, 2);
    canSlowControl->cd(1);
    gCh3_v->Draw("ALP");
    canSlowControl->cd(2);
    gCh3_c->Draw("ALP");
    canSlowControl->cd(3);
    gCh4_v->Draw("ALP");
    canSlowControl->cd(4);
    gCh4_c->Draw("ALP");

    Double_t Tps = 1E12 / 200.e6;
    Float_t Tns = Tps / 1.e3;
    while(true) {
        if (gSystem->ProcessEvents() ) break;
        zmq::message_t msg;
        try {
            //message from the zmq publisher
            sub.recv(msg, zmq::recv_flags::dontwait);
        } catch(zmq::error_t &e) {
            fprintf(stderr, "%s\n", e.what() );
        }
        if(msg.size() ) {
            //parsing string object into json object
            std::string readBuffer = msg.to_string();
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(readBuffer);
            } catch(nlohmann::json::parse_error &e) {
                fprintf(stderr, "%d %s\n", e.id, e.what() );
            }
            hFrames->Fill(j["id"].get<int>() );
            hFramesLost->Fill(j["lost"].get<int>() );
            nlohmann::json ev = j["events"];
            for(UShort_t x=0; x < ev.size(); ++x) {
                int channelID = ev[x][0].get<int>();
                // Efine
                vecQDC[channelID]->Fill(ev[x][5].get<int>() / Tns );
                // Time
                vecTime[channelID]->Fill(ev[x][4].get<int>() * Tns); // ns
                if(address[channelID]["s"].compare("l") == 0) {
                    //fill hits on the left
                    hHitsL->Fill(std::stoi(address[channelID]["f"]), std::stoi(address[channelID]["l"]) );
                }
                if(address[channelID]["s"].compare("r") == 0) {
                    //fill hits on the right
                    hHitsR->Fill(std::stoi(address[channelID]["f"]), std::stoi(address[channelID]["l"]) );
                }
            }
        }
        zmq::message_t msg0;
        try {
            //message from the zmq publisher
            sub0.recv(msg0, zmq::recv_flags::dontwait);
        } catch(zmq::error_t &e) {
            fprintf(stderr, "%s\n", e.what() );
        }
        if(msg0.size() ) {
            std::string readBuffer = msg0.to_string();
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(readBuffer);
            } catch(nlohmann::json::parse_error &e) {
                fprintf(stderr, "%d %s\n", e.id, e.what() );
            }
            gCh3_c->AddPoint(j["timestamp"].get<double>(), j["ch3_c"].get<double>() );
            gCh3_v->AddPoint(j["timestamp"].get<double>(), j["ch3_v"].get<double>() );
            gCh4_c->AddPoint(j["timestamp"].get<double>(), j["ch4_c"].get<double>() );
            gCh4_v->AddPoint(j["timestamp"].get<double>(), j["ch4_v"].get<double>() );
        }
    }
    fOutput->Write();
    fOutput->Close();
    return 0;
}
