#include <zmq.hpp>
#include "nlohmann/json.hpp"
#include <TROOT.h>
#include <TSystem.h>
#include <TFile.h>
#include <TKey.h>
#include <TH1.h>
#include <TH2.h>
#include <TGraph.h>
#include <THttpServer.h>
#include "TRootSniffer.h"
#include <TCanvas.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <signal.h>

#include "CommandLine.h"
volatile sig_atomic_t terminate;
void signal_handler(int signum) {
    terminate = kTRUE;
}
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
TGraph * CreateGraph(const char * name, const char *title) {
    TGraph *g = new TGraph();
    g->SetName(name);
    g->SetTitle(title);
    g->GetXaxis()->SetTimeDisplay(1);
    g->GetXaxis()->SetTimeFormat("#splitline{%b %d}{%H:%M:%S}");
    g->GetYaxis()->SetMaxDigits(3);
    gDirectory->Add(g);
    return g;
}
void Process(const char *filename, ARGUMENTS arguments) {
    TFile *fOutput = new TFile(filename, "RECREATE");
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
    TGraph *gCh3_c = CreateGraph("gCh3_c", "Ch3 current; ;I[A]");
    TGraph *gCh3_v = CreateGraph("gCh3_v", "Ch3 voltage; ;V[V]");
    TGraph *gCh4_c = CreateGraph("gCh4_c", "Ch4 current; ;I[A]");
    TGraph *gCh4_v = CreateGraph("gCh4_v", "Ch4 voltage; ;V[V]");
    
    gDirectory->cd("/");
    gDirectory->mkdir("feba");
    gDirectory->cd("/feba");
    TGraph *gCh0_0_T = CreateGraph("gCh0_0_T", "FEB/A 0_0; ;T[^{o}C]");
    TGraph *gCh0_1_T = CreateGraph("gCh0_1_T", "FEB/A 0_1; ;T[^{o}C]");
    TGraph *gCh2_0_T = CreateGraph("gCh2_0_T", "FEB/A 2_0; ;T[^{o}C]");
    TGraph *gCh2_1_T = CreateGraph("gCh2_1_T", "FEB/A 2_1; ;T[^{o}C]");

    gDirectory->cd("/");
    gDirectory->mkdir("localmachine");
    gDirectory->cd("/localmachine");
    TGraph *gDiskUsage = CreateGraph("gDiskUsage", "Disk usage; ;[%]");
    TGraph *gIOReadCount = CreateGraph("gIOReadCount", "IO read count");
    TGraph *gIOWriteCount = CreateGraph("gIOWriteCount", "IO write count");

    // connect to the zmq publisher created in the daq machine
    zmq::context_t ctx;
    zmq::socket_t sub(ctx, ZMQ_SUB);
    sub.connect("tcp://172.16.32.214:2000");
    sub.set(zmq::sockopt::subscribe, "");
    // connect to the zmq publisher created in the devices
    zmq::socket_t sub0(ctx, ZMQ_SUB);
    sub0.connect("tcp://172.16.32.214:2001");
    sub0.set(zmq::sockopt::subscribe, "");
    zmq::socket_t sub1(ctx, ZMQ_SUB);
    sub1.connect("tcp://172.16.32.214:2002");
    sub1.set(zmq::sockopt::subscribe, "");
    zmq::socket_t sub2(ctx, ZMQ_SUB);
    sub2.connect("tcp://172.16.32.214:2003");
    sub2.set(zmq::sockopt::subscribe, "");
    
    //start the monitoring server locally
    // const char *url = "http:127.0.0.1:8888";
    const char *url = "http:172.16.32.214:8888";
    THttpServer *server = new THttpServer(url);
//    server->RegisterCommand("/Start", "SControl::Start()", "button;rootsys/icons/ed_execute.png");
//    server->RegisterCommand("/Stop",  "SControl::Stop()", "button;rootsys/icons/ed_interrupt.png");

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
    TCanvas *canHits = new TCanvas("canHits", "canHits");
    canHits->Divide(1, 2);
    canHits->cd(1);
    hHitsL->Draw("COLZ");
    canHits->cd(2);
    hHitsR->Draw("COLZ");

    TCanvas *canSlowControl = new TCanvas("canSlowControl", "canSlowControl");
    canSlowControl->Divide(4, 3);
    canSlowControl->cd(1);
    gCh3_v->Draw("ALP");
    canSlowControl->cd(2);
    gCh3_c->Draw("ALP");
    canSlowControl->cd(3);
    gCh4_v->Draw("ALP");
    canSlowControl->cd(4);
    gCh4_c->Draw("ALP");
    canSlowControl->cd(5);
    gCh0_0_T->Draw("ALP");
    canSlowControl->cd(6);
    gCh0_1_T->Draw("ALP");
    canSlowControl->cd(7);
    gCh2_0_T->Draw("ALP");
    canSlowControl->cd(8);
    gCh2_1_T->Draw("ALP");
    canSlowControl->cd(9);
    gDiskUsage->Draw("ALP");
    canSlowControl->cd(10);
    gIOReadCount->Draw("ALP");
    canSlowControl->cd(11);
    gIOWriteCount->Draw("ALP");

    Double_t Tps = 1E12 / 200.e6;
    Float_t Tns = Tps / 1.e3;
    const std::chrono::time_point before = std::chrono::steady_clock::now();

    zmq::recv_result_t res = 0;
    while(!terminate) {
        if (arguments.snapshot_type.compare("seconds") == 0 && arguments.snapshot_interval != 0) {
            if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - before).count() > arguments.snapshot_interval) break;
        }
        if (arguments.snapshot_type.compare("events") == 0) {
            // TODO:
        }
        if (gSystem->ProcessEvents() ) break;
        zmq::message_t msg;
        try {
            //message from the zmq publisher
            res = sub.recv(msg, zmq::recv_flags::dontwait);
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
        try {
            //message from the zmq publisher
            res = sub0.recv(msg, zmq::recv_flags::dontwait);
        } catch(zmq::error_t &e) {
            fprintf(stderr, "%s\n", e.what() );
        }
        if(msg.size() ) {
            std::string readBuffer = msg.to_string();
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
        try {
            //message from the zmq publisher
            res = sub1.recv(msg, zmq::recv_flags::dontwait);
        } catch(zmq::error_t &e) {
            fprintf(stderr, "%s\n", e.what() );
        }
        if(msg.size() ) {
            std::string readBuffer = msg.to_string();
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(readBuffer);
            } catch(nlohmann::json::parse_error &e) {
                fprintf(stderr, "%d %s\n", e.id, e.what() );
            }
            gCh0_0_T->AddPoint(j["timestamp"].get<double>(), j["ch0_0_T"].get<double>() );
            gCh0_1_T->AddPoint(j["timestamp"].get<double>(), j["ch0_1_T"].get<double>() );
            gCh2_0_T->AddPoint(j["timestamp"].get<double>(), j["ch2_0_T"].get<double>() );
            gCh2_1_T->AddPoint(j["timestamp"].get<double>(), j["ch2_1_T"].get<double>() );
        }
        try {
            //message from the zmq publisher
            res = sub2.recv(msg, zmq::recv_flags::dontwait);
        } catch(zmq::error_t &e) {
            fprintf(stderr, "%s\n", e.what() );
        }
        if(msg.size() ) {
            std::string readBuffer = msg.to_string();
            nlohmann::json j;
            try {
                j = nlohmann::json::parse(readBuffer);
            } catch(nlohmann::json::parse_error &e) {
                fprintf(stderr, "%d %s\n", e.id, e.what() );
            }
            gDiskUsage->AddPoint(j["timestamp"].get<double>(), j["diskusage"].get<double>() );
            gIOReadCount->AddPoint(j["timestamp"].get<double>(), j["iowritecount"].get<double>() );
            gIOWriteCount->AddPoint(j["timestamp"].get<double>(), j["ioreadcount"].get<double>() );
        }
    }
    fOutput->Write();
    fOutput->Close();

    delete canQDCR;
    delete canQDCL;
    delete canTimeR;
    delete canTimeL;
    delete canHits;
    delete canSlowControl;
    delete server;
    delete fOutput;
}
int main(int argc, char *argv[]) {
    ARGUMENTS arguments;
    int ret = analyze_command_line(argc, argv, arguments);
    print_arguments(arguments);
    if(ret != 0) return ret;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    UShort_t counter = 0;
    while(!terminate) {
        std::string filename = "monitoring" + std::to_string(counter) + ".root";
        Process(filename.c_str(), arguments);
        counter++;
    }
    return 0;
}
