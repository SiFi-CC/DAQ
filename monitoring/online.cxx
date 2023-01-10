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
//#include "Timer.h"
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
TGraph * CreateGraph(std::string name, std::string title) {
    TGraph *g = new TGraph();
    g->SetName(name.c_str() );
    g->SetTitle(title.c_str() );
    g->GetXaxis()->SetTimeDisplay(1);
    g->GetXaxis()->SetTimeFormat("#splitline{%b %d}{%H:%M:%S}");
    g->GetYaxis()->SetMaxDigits(3);
    gDirectory->Add(g);
    return g;
}
void InitGraphs(std::map<std::string, TGraph *> &mapGraphs, nlohmann::json jModule) {
    for (nlohmann::json::iterator it = jModule.begin(); it != jModule.end(); ++it) {
        nlohmann::json obj = *it;
        gDirectory->cd("/");
        gDirectory->mkdir(Form("%s", obj["dir"].get<std::string>().c_str() ) );
        gDirectory->cd(Form("/%s", obj["dir"].get<std::string>().c_str() ) );
        for (nlohmann::json::iterator it = obj["contents"].begin(); it != obj["contents"].end(); ++it) {
            nlohmann::json obj = *it;
            mapGraphs[obj["name"].get<std::string>()] = CreateGraph(obj["name"].get<std::string>(), obj["title"].get<std::string>() );
        }
    }

}
void ParseModule(std::ifstream& in, nlohmann::json &j) {
    std::ostringstream sstr;
    sstr << in.rdbuf();
    j = nlohmann::json::parse(sstr.str() );
}
void Process(ARGUMENTS arguments, nlohmann::json jModule) {
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
    for(UInt_t i=0; i < 256; ++i) {
        std::map<std::string, std::string> m = {{"m",""}, {"l",""}, {"f",""}, {"s",""} };
        address.push_back(m);
        //create vector of histograms to fill qdc and time
        vecQDC.push_back(new TH1F(Form("hQDC%d", i), Form("%d;qdc", i), 10, 0, 100) ); //qdc 
        vecTime.push_back(new TH1F(Form("hTime%d", i), Form("%d;t[ns]", i), 100, 0, 10000) ); //time 
    }
//    findFiberAddress("../sandbox/sifi_params.txt", address);

//    gDirectory->cd("/");
//    gDirectory->mkdir("hits");
//    gDirectory->cd("/hits");
//    TH2S *hHitsL = new TH2S("hHitsL", "L;fiber;layer", 16, 0, 16, 4, 0, 4);
//    TH2S *hHitsR = new TH2S("hHitsR", "R;fiber;layer", 16, 0, 16, 4, 0, 4);
//    hHitsL->SetOption("COLZ");
//    hHitsR->SetOption("COLZ");

    // connect to the zmq publisher created in the daq machine
    zmq::context_t ctx;
//    zmq::socket_t sub(ctx, ZMQ_SUB);
//    sub.connect("tcp://172.16.32.214:2000");
//    sub.setsockopt(ZMQ_SUBSCRIBE, "", 0);
//    // connect to the zmq publisher created in the devices
//    zmq::socket_t sub0(ctx, ZMQ_SUB);
//    sub0.connect("tcp://172.16.32.214:2001");
//    sub0.setsockopt(ZMQ_SUBSCRIBE, "", 0);
//    zmq::socket_t sub1(ctx, ZMQ_SUB);
//    sub1.connect("tcp://172.16.32.214:2002");
//    sub1.setsockopt(ZMQ_SUBSCRIBE, "", 0);
//    zmq::socket_t sub2(ctx, ZMQ_SUB);
//    sub2.connect("tcp://172.16.32.214:2003");
//    sub2.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    zmq::socket_t sub3(ctx, ZMQ_SUB);
    sub3.connect("tcp://172.16.32.214:2004");
    sub3.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    
    //start the monitoring server locally
    // const char *url = "http:127.0.0.1:8888";
    const char *url = "http:172.16.32.214:8888";
    THttpServer *server = new THttpServer(url);
//    server->RegisterCommand("/Start", "SControl::Start()", "button;rootsys/icons/ed_execute.png");
//    server->RegisterCommand("/Stop",  "SControl::Stop()", "button;rootsys/icons/ed_interrupt.png");

//    TCanvas *canQDCR = new TCanvas("canQDCR", "canQDCR");
//    canQDCR->Divide(16, 4);
//    for(UInt_t i=0; i < 16; ++i) {
//        for(UInt_t j=0; j < 4; ++j) {
//            canQDCR->cd(1+ i + 16*j);
//            vecQDC[i + 16*j]->Draw();
//        }
//    }
//    TCanvas *canQDCL = new TCanvas("canQDCL", "canQDCL");
//    canQDCL->Divide(16, 4);
//    for(UInt_t i=0; i < 16; ++i) {
//        for(UInt_t j=0; j < 4; ++j) {
//            canQDCL->cd(1+ i + 16*j);
//            vecQDC[256 + i + 16*j]->Draw();
//        }
//    }
//    TCanvas *canTimeR = new TCanvas("canTimeR", "canTimeR");
//    canTimeR->Divide(16, 4);
//    for(UInt_t i=0; i < 16; ++i) {
//        for(UInt_t j=0; j < 4; ++j) {
//            canTimeR->cd(1+ i + 16*j);
//            vecTime[i + 16*j]->Draw();
//        }
//    }
//    TCanvas *canTimeL = new TCanvas("canTimeL", "canTimeL");
//    canTimeL->Divide(16, 4);
//    for(UInt_t i=0; i < 16; ++i) {
//        for(UInt_t j=0; j < 4; ++j) {
//            canTimeL->cd(1+ i + 16*j);
//            vecTime[256 + i + 16*j]->Draw();
//        }
//    }
//    TCanvas canHits("canHits", "canHits");
//    canHits.Divide(1, 2);
//    canHits.cd(1);
//    hHitsL->Draw("COLZ");
//    canHits.cd(2);
//    hHitsR->Draw("COLZ");
    
    // Slow Control TCanvas
    std::map<std::string, TGraph *> mapGraphs;
    InitGraphs(mapGraphs, jModule);
    TCanvas *canSlowControl = new TCanvas("canSlowControl", "canSlowControl");
    canSlowControl->Divide(4, 3);
    UShort_t counter = 1;
    for(std::map<std::string, TGraph *>::iterator it = mapGraphs.begin(); it != mapGraphs.end(); ++it) {
        canSlowControl->cd(counter);
        it->second->Draw("ALP");
        counter++;
    }

    Double_t Tps = 1E12 / 200.e6;
    Float_t Tns = Tps / 1.e3;

    // Do stuff at every interval
//    Timer t = Timer();
//    Int_t lastN = 0;
//    t.setInterval([&]() {
//        //Copy contents from main TGraphs but in intervals, starting from the last read Nth entry
//        TFile *fInt = new TFile(Form("inter%d.root", lastN), "RECREATE");
//        std::map<std::string, TGraph *> _mapGraphs;
//        InitGraphs(_mapGraphs, jModule);
//        for(std::map<std::string, TGraph *>::iterator it = _mapGraphs.begin(); it != _mapGraphs.end(); ++it) {
//            for(UShort_t i=lastN; i < mapGraphs[it->first]->GetN(); ++i) {
//                _mapGraphs[it->first]->SetPoint(i-lastN, mapGraphs[it->first]->GetX()[i], mapGraphs[it->first]->GetY()[i]);
//            }
//        }
//        lastN = mapGraphs["gCh1_0_T"]->GetN();
//        if(fInt->Write() ) {
//            printf("Intermediate file %s written.\n", fInt->GetName() );
//        } else {
//            fprintf(stderr, "Error writing to intermediate file.\n");
//        }
//        fInt->Close();
//        delete fInt;
//
//    }, arguments.snapshot_interval);

    zmq::recv_result_t res = 0;
    while(!terminate) {
        if (gSystem->ProcessEvents() ) break;
        zmq::message_t msg;
//        try {
//            //message from the zmq publisher
//            res = sub.recv(msg, zmq::recv_flags::dontwait);
//        } catch(zmq::error_t &e) {
//            fprintf(stderr, "%s\n", e.what() );
//        }
//        if(msg.size() ) {
//            //parsing string object into json object
//            std::string readBuffer = msg.to_string();
//            nlohmann::json j;
//            try {
//                j = nlohmann::json::parse(readBuffer);
//            } catch(nlohmann::json::parse_error &e) {
//                fprintf(stderr, "%d %s\n", e.id, e.what() );
//            }
//            hFrames->Fill(j["id"].get<int>() );
//            hFramesLost->Fill(j["lost"].get<int>() );
//            nlohmann::json ev = j["events"];
//            for(UShort_t x=0; x < ev.size(); ++x) {
//                int ch = ev[x][0].get<int>();
//                int channelID = ch - 131200;
//                // Efine
//                vecQDC[channelID]->Fill(ev[x][5].get<int>() / Tns );
//                // Time
//                vecTime[channelID]->Fill(ev[x][4].get<int>() * Tns); // ns
//                if(address[channelID]["s"].compare("l") == 0) {
//                    //fill hits on the left
//                    hHitsL->Fill(std::stoi(address[channelID]["f"]), std::stoi(address[channelID]["l"]) );
//                }
//                if(address[channelID]["s"].compare("r") == 0) {
//                    //fill hits on the right
//                    hHitsR->Fill(std::stoi(address[channelID]["f"]), std::stoi(address[channelID]["l"]) );
//                }
//            }
//        }
//        try {
//            //message from the zmq publisher
//            res = sub0.recv(msg, zmq::recv_flags::dontwait);
//        } catch(zmq::error_t &e) {
//            fprintf(stderr, "%s\n", e.what() );
//        }
//        if(msg.size() ) {
//            std::string readBuffer = msg.to_string();
//            nlohmann::json j;
//            try {
//                j = nlohmann::json::parse(readBuffer);
//            } catch(nlohmann::json::parse_error &e) {
//                fprintf(stderr, "%d %s\n", e.id, e.what() );
//            }
//            mapGraphs["gCh3_c"]->SetPoint(mapGraphs["gCh3_c"]->GetN(), j["timestamp"].get<double>(), j["ch3_c"].get<double>() );
//            mapGraphs["gCh3_v"]->SetPoint(mapGraphs["gCh3_v"]->GetN(), j["timestamp"].get<double>(), j["ch3_v"].get<double>() );
//            mapGraphs["gCh4_c"]->SetPoint(mapGraphs["gCh3_c"]->GetN(), j["timestamp"].get<double>(), j["ch4_c"].get<double>() );
//            mapGraphs["gCh4_v"]->SetPoint(mapGraphs["gCh3_v"]->GetN(), j["timestamp"].get<double>(), j["ch4_v"].get<double>() );
//        }
//        try {
//            //message from the zmq publisher
//            res = sub1.recv(msg, zmq::recv_flags::dontwait);
//        } catch(zmq::error_t &e) {
//            fprintf(stderr, "%s\n", e.what() );
//        }
//        if(msg.size() ) {
//            std::string readBuffer = msg.to_string();
//            nlohmann::json j;
//            try {
//                j = nlohmann::json::parse(readBuffer);
//            } catch(nlohmann::json::parse_error &e) {
//                fprintf(stderr, "%d %s\n", e.id, e.what() );
//            }
//            mapGraphs["gCh1_0_T"]->SetPoint(mapGraphs["gCh1_0_T"]->GetN(), j["timestamp"].get<double>(), j["ch1_0_T"].get<double>() );
//            mapGraphs["gCh1_1_T"]->SetPoint(mapGraphs["gCh1_1_T"]->GetN(), j["timestamp"].get<double>(), j["ch1_1_T"].get<double>() );
//            mapGraphs["gCh2_0_T"]->SetPoint(mapGraphs["gCh2_0_T"]->GetN(), j["timestamp"].get<double>(), j["ch2_0_T"].get<double>() );
//            mapGraphs["gCh2_1_T"]->SetPoint(mapGraphs["gCh2_1_T"]->GetN(), j["timestamp"].get<double>(), j["ch2_1_T"].get<double>() );
//        }
//        try {
//            //message from the zmq publisher
//            res = sub2.recv(msg, zmq::recv_flags::dontwait);
//        } catch(zmq::error_t &e) {
//            fprintf(stderr, "%s\n", e.what() );
//        }
//        if(msg.size() ) {
//            std::string readBuffer = msg.to_string();
//            nlohmann::json j;
//            try {
//                j = nlohmann::json::parse(readBuffer);
//            } catch(nlohmann::json::parse_error &e) {
//                fprintf(stderr, "%d %s\n", e.id, e.what() );
//            }
//            mapGraphs["gDiskUsage"]->SetPoint(mapGraphs["gDiskUsage"]->GetN(), j["timestamp"].get<double>(), j["diskusage"].get<double>() );
//            mapGraphs["gIOReadCount"]->SetPoint(mapGraphs["gIOReadCount"]->GetN(), j["timestamp"].get<double>(), j["iowritecount"].get<double>() );
//            mapGraphs["gIOWriteCount"]->SetPoint(mapGraphs["gIOWriteCount"]->GetN(), j["timestamp"].get<double>(), j["ioreadcount"].get<double>() );
//        }
        try {
            //message from the zmq publisher
            res = sub3.recv(msg, zmq::recv_flags::dontwait);
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
            mapGraphs["28.A953460B0000"]->SetPoint(mapGraphs["28.A953460B0000"]->GetN(), j["timestamp"].get<double>(), j["28.A953460B0000"].get<double>() );
            mapGraphs["28.2D28440B0000"]->SetPoint(mapGraphs["28.2D28440B0000"]->GetN(), j["timestamp"].get<double>(), j["28.2D28440B0000"].get<double>() );
	    // issue with the 1-wire sensor that sends NULL
	    if(j["28.8B08470B0000"].get<double>() > 0)
            	mapGraphs["28.8B08470B0000"]->SetPoint(mapGraphs["28.8B08470B0000"]->GetN(), j["timestamp"].get<double>(), j["28.8B08470B0000"].get<double>() );

        }
    }
    fOutput->Write();
    fOutput->Close();

//    delete canQDCR;
//    delete canQDCL;
//    delete canTimeR;
//    delete canTimeL;
//    delete server;
//    delete fOutput;
}
int main(int argc, char *argv[]) {
    ARGUMENTS arguments;
    int ret = analyze_command_line(argc, argv, arguments);
    print_arguments(arguments);
    if(ret != 0) return ret;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    std::ifstream fModule;
    fModule.open("MODULES");
    nlohmann::json j;
    ParseModule(fModule, j);
    Process(arguments, j);
    return 0;
}
