#include <shm_raw.hpp>
#include <RawReader.hpp>
#include <OrderedEventHandler.hpp>
#include <getopt.h>
#include <assert.h>

#include <boost/lexical_cast.hpp>

#include <TFile.h>
#include <TTree.h>
#include <TNtuple.h>

#include <SCategory.h>
#include <SLocator.h>
#include <SFibersRaw.h>
#include <SDataSource.h>
#include <SUnpacker.h>
#include <SDatabase.h>
#include <SDetectorManager.h>
#include <SFibersDetector.h>
#include <SFibersLookup.h>
#include <SParAsciiSource.h>
#include <STaskManager.h>
#include <SiFi.h>

using namespace PETSYS;
struct TPHit
{
    UInt_t time;
    UInt_t energy;
    UInt_t channelID;
    void print() const {
        printf("TOFPET: time = %d, energy = %d, channelID = %d\n", time, energy, channelID);
    }
};
class RawUnpacker : public SUnpacker {
protected:
    SCategory* catFibersRaw;
    SCategory* catFibersCal;
    SFibersLookupTable* pLookUp;
public:
    RawUnpacker() {};
    bool init() {
        SUnpacker::init();
        catFibersRaw = sifi()->buildCategory(SCategory::CatFibersRaw);
        if (!catFibersRaw) {
            std::cerr << "No CatFibersRaw category" << std::endl;
            return false;
        }
        catFibersCal = sifi()->buildCategory(SCategory::CatFibersCal);
        if (!catFibersCal) {
            std::cerr << "No CatFibersCal category" << std::endl;
            return false;
        }
        pLookUp = dynamic_cast<SFibersLookupTable*>(pm()->getLookupContainer("FibersTPLookupTable"));
        return true;
    }
    bool execute(ulong event, ulong seq_number, uint16_t subevent, void* buffer, size_t length) {
        TPHit* hit = static_cast<TPHit*>(buffer);
        if (!hit) return false;
        SFibersChannel* lc = dynamic_cast<SFibersChannel*>(pLookUp->getAddress(0x1000, hit->channelID));
        if (!lc) {
            std::cerr << "channel " << hit->channelID << " information does not exist in " << pLookUp->GetName() << std::endl;
        } else {
            SLocator loc(3);
            loc[0] = lc->m; // mod;
            loc[1] = lc->l; // lay;
            loc[2] = lc->s; // fib;
            char side = lc->side;
            SFibersRaw* pRaw = dynamic_cast<SFibersRaw*>(catFibersRaw->getObject(loc));
            if (!pRaw) {
                pRaw = reinterpret_cast<SFibersRaw*>(catFibersRaw->getSlot(loc));
                new (pRaw) SFibersRaw;
            }
            pRaw->setAddress(loc[0], loc[1], loc[2]);
            if(side == 'l') {
                pRaw->setQDCL(hit->energy);
                pRaw->setTimeL(hit->time);
                pRaw->setQDCR(-100);
                pRaw->setTimeR(-100);
            }
            else if(side == 'r'){
                pRaw->setQDCL(-100);
                pRaw->setTimeL(-100);
                pRaw->setQDCR(hit->energy);
                pRaw->setTimeR(hit->time);
            }
            else {
                std::cerr << "fiber side undefined!" << std::endl;
            }
        }
        return true;
    }
};
class RawDataSource : public SDataSource {
    public:
        RawDataSource() { };
        RawDataSource(std::vector<std::shared_ptr<TPHit> > buffer) :
        _buffer(buffer)
        { };
        bool open() { 
            return unpackers[0x1000]->init();
        };
        bool close() { 
            return unpackers[0x1000]->finalize();
        };
        bool readCurrentEvent() {
            std::shared_ptr<TPHit> hit = _buffer[getCurrentEvent() ];
            for (const auto& u : unpackers) {
                u.second->setSampleTimeBin(1.);
                u.second->setADCTomV(1.);
                u.second->execute(0, 0, u.first, hit.get(), 0);
            }
            return true; 
        };
        void setInput(const std::string& filename, size_t length = 0) {};
        std::vector<std::shared_ptr<TPHit> > _buffer;
};
class DataFileWriter {
public:
	DataFileWriter() { };
	void addEvents(PETSYS::EventBuffer<PETSYS::RawHit> *buffer) {
        for(int i=0; i < buffer->getSize(); i++) {
            PETSYS::RawHit &hit = buffer->get(i);
            std::shared_ptr<TPHit> hit_cache = std::make_shared<TPHit>();
            hit_cache->channelID = hit.channelID;
            hit_cache->time = hit.tfine;
            hit_cache->energy = hit.efine;
            _hits.push_back(hit_cache);
        }
	};
    std::vector<std::shared_ptr<TPHit> > getEvents() { 
        return _hits;
    };
    std::vector<std::shared_ptr<TPHit> > _hits;
    PETSYS::EventBuffer<PETSYS::RawHit> * _buffer;
};
class WriteHelper : public OrderedEventHandler<PETSYS::RawHit, PETSYS::RawHit> {
    public:
        DataFileWriter *dataFileWriter;
        WriteHelper(DataFileWriter *dataFileWriter, PETSYS::EventSink<PETSYS::RawHit> *sink) :
            OrderedEventHandler<PETSYS::RawHit, PETSYS::RawHit>(sink),
    		dataFileWriter(dataFileWriter)
            { };
        PETSYS::EventBuffer<PETSYS::RawHit> * handleEvents(PETSYS::EventBuffer<PETSYS::RawHit> *buffer) { 
            dataFileWriter->addEvents(buffer);
            return buffer;
        };
        void pushT0(double t0) { };
        void report() { };
};

int main(int argc, char *argv[] ) {
    char *configFileName = NULL;
    char *inputFilePrefix = NULL;
    char *outputFileName = NULL;
    long long eventFractionToWrite = 1024;
    static struct option longOptions[] = {
        { "help", no_argument, 0, 0 },
        { "config", required_argument, 0, 0 },
        { "writeFraction", required_argument }
    };
    while(true) {
        int optionIndex = 0;
        int c = getopt_long(argc, argv, "i:o:",longOptions, &optionIndex);
        if(c == -1) break;
        else if(c != 0) {
            // Short arguments
            switch(c) {
                case 'i':       inputFilePrefix = optarg; break;
                case 'o':       outputFileName = optarg; break;
            }
        }
        else if(c == 0) {
            switch(optionIndex) {
                case 1:		configFileName = optarg; break;
                case 2:		eventFractionToWrite = round(1024 *boost::lexical_cast<float>(optarg) / 100.0); break;
            }
        }
        else {
            assert(false);
        }
    }
    if(configFileName == NULL) {
        fprintf(stderr, "--config must be specified\n");
        exit(1);
    }
    if(inputFilePrefix == NULL) {
        fprintf(stderr, "-i must be specified\n");
        exit(1);
    }
    if(outputFileName == NULL) {
        fprintf(stderr, "-o must be specified\n");
        exit(1);
    }
    RawReader *reader = RawReader::openFile(inputFilePrefix);
	DataFileWriter *dataFileWriter = new DataFileWriter();
    WriteHelper *writeHelper = new WriteHelper(dataFileWriter, new PETSYS::NullSink<PETSYS::RawHit>() );
    reader->processStep(0, true, writeHelper);

    RawUnpacker * unp = new RawUnpacker();
    RawDataSource * source = new RawDataSource(dataFileWriter->getEvents() );
    source->addUnpacker(unp, {0x1000});
    sifi()->addSource(source);
    sifi()->setOutputFileName("test.root");
    sifi()->book();
    pm()->addSource(new SParAsciiSource("sifi_params.txt") );
    // initialize detectors
    SDetectorManager* detm = SDetectorManager::instance();
    detm->addDetector(new SFibersDetector("Fibers"));
    detm->initTasks();
    detm->initParameterContainers();
    detm->initCategories();
    pm()->addLookupContainer("FibersTPLookupTable", std::make_unique<SFibersLookupTable>("FibersTPLookupTable", 0x1000, 0x1fff, 5000));
    // initialize parameters
    STaskManager* tm = STaskManager::instance();
    tm->initTasks();
    sifi()->setTree(new TTree() );
    sifi()->loop(reader->getTotalEvents() );
    tm->finalizeTasks();
    sifi()->save();
    return 0;
}
