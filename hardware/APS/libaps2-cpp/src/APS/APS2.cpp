#include "APS2.h"
#include <sstream>

using std::ostringstream;
using std::endl;


string APS2::printStatusRegisters(const APS_Status_Registers & status) {
	ostringstream ret;

	ret << "Host Firmware Version = " << std::hex << status.hostFirmwareVersion << endl;
	ret << "User Firmware Version = " << std::hex << status.userFirmwareVersion << endl;
	ret << "Configuration Source  = " << status.configurationSource << endl;
	ret << "User Status           = " << status.userStatus << endl;
	ret << "DAC 0 Status          = " << status.dac0Status << endl;
	ret << "DAC 1 Status          = " << status.dac1Status << endl;
	ret << "PLL Status            = " << status.pllStatus << endl;
	ret << "VCXO Status           = " << status.vcxoStatus << endl;
	ret << "Send Packet Count     = " << status.sendPacketCount << endl;
	ret << "Recv Packet Count     = " << status.receivePacketCount << endl;
	ret << "Seq Skip Count        = " << status.sequenceSkipCount << endl;
	ret << "Seq Dup  Count        = " << status.sequenceDupCount << endl;
	ret << "Uptime                = " << status.uptime << endl;
	ret << "Reserved 1            = " << status.reserved1 << endl;
	ret << "Reserved 2            = " << status.reserved2 << endl;
	ret << "Reserved 3            = " << status.reserved3 << endl;
	return ret.str();
}

string APS2::printAPSCommand(const APSCommand_t & cmd) {
    ostringstream ret;

    ret << std::hex << cmd.packed << " =";
    ret << " ACK: " << cmd.ack;
    ret << " SEQ: " << cmd.seq;
    ret << " SEL: " << cmd.sel;
    ret << " R/W: " << cmd.r_w;
    ret << " CMD: " << cmd.cmd;
    ret << " MODE/STAT: " << cmd.mode_stat;
    ret << std::dec << " cnt: " << cmd.cnt;
    return ret.str();
}

string APS2::printAPSChipCommand(APSChipConfigCommand_t & cmd) {
    ostringstream ret;

    ret << std::hex << cmd.packed << " =";
    ret << " Target: " << cmd.target;
    ret << " SPICNT_DATA: " << cmd.spicnt_data;
    ret << " INSTR: " << cmd.instr;
    return ret.str();
}


uint32_t * APS2::getPayloadPtr(uint32_t * frame) {
   frame += sizeof(APS2EthernetHeader) / sizeof(uint32_t);
   return frame;
}


APS2::APS2() :  isOpen{false}, deviceID_{-1}, channels_(2), samplingRate_{-1}, writeQueue_(0), streaming_{false})} {}

APS2::APS2(int deviceID, string deviceSerial) :  isOpen{false}, deviceID_{deviceID}, deviceSerial_{deviceSerial},
		samplingRate_{-1}, writeQueue_(0) {
			channels_.reserve(2);
			for(size_t ct=0; ct<2; ct++) channels_.push_back(Channel(ct));
};

APS2::~APS2() = default;

int APS2::connect(){
	if (!isOpen) {
		int success = 0;

		success = handle_.connect(deviceSerial_);

		if (success == 0) {
			FILE_LOG(logINFO) << "Opened connection to device " << deviceID_ << " (Serial: " << deviceSerial_ << ")";
			isOpen = true;
		}
		// TODO: restore state information from file
		return success;
	}
	else {
		return 0;
	}
}

int APS2::disconnect(){
	if (isOpen){
		int success = 0;
		success = handle_.disconnect();
		if (success == 0) {
			FILE_LOG(logINFO) << "Closed connection to device " << deviceID_ << " (Serial: " << deviceSerial_ << ")";
			isOpen = false;
		}
		// TODO: save state information to file
		return success;
	}
	else{
		return 0;
	}
}

int APS2::reset() {
	APSCommand_t command;
	command.packed = 0;
	
	command.cmd = APS_COMMAND_RESET;
	command.mode_stat = RESET_RECONFIG_BASELINE_EPROM;
	handle_.write(command);

	// get reply with status bytes
	
	struct APS2_Status_Registers statusRegs;
	handle_.read(&statusRegs, sizeof(struct APS2_Status_Registers));

	FILE_LOG(logDEBUG1) << 	APS2::printStatusRegisters(statusRegs);
	return 0;
}

int APS2::init(const bool & forceReload, const bool & int bitFileNum){
	 //TODO: bitfiles will be stored in flash so all we need to do here is the DAC's

	if (forceReload || !read_PLL_status()) {
		FILE_LOG(logINFO) << "Resetting instrument";
		FILE_LOG(logINFO) << "Found force: " << forceReload << " bitFile version: " << myhex << read_bitFile_version() << " PLL status: " << read_PLL_status();

		// send hard reset to APS2
		// this will reconfigure the DACs, PLL and VCX0 with EPROM settings
		reset()

		
		//Program the bitfile to both FPGA's
		program_bitfile(bitFileNum);

		//Default to max sample rate
		set_sampleRate(1200);

		// test PLL sync on each FPGA
		//int status = test_PLL_sync(FPGA1);
		int status = test_PLL_sync();
		if (status) {
			FILE_LOG(logERROR) << "DAC PLLs failed to sync";
		}

		// align DAC data clock boundaries
		setup_DACs();

		// clear channel data
		clear_channel_data();
	}
	samplingRate_ = get_sampleRate();

	return 0;
}

int APS2::setup_DACs() {
	//Call the setup function for each DAC
	for(int dac=0; dac < NUM_CHANNELS; dac++){
		setup_DAC(dac);
	}
	return 0;
}


int APS2::program_FPGA(const string & bitFile, const int & expectedVersion) {
	/**
	 * @param bitFile path to a Lattice bit file
	 * @param chipSelect which FPGA to write to (FPGA1, FPGA2, BOTH_FGPAS)
	 * @param expectedVersion - checks whether version register matches this value after programming. -1 = skip the check
	 */

	//Open the bitfile
	//
	FILE_LOG(logDEBUG) << "Opening bitfile: " << bitFile;
	std::ifstream FID (bitFile, std::ios::in|std::ios::binary);
	//Get the size
	if (!FID.is_open()){
		FILE_LOG(logERROR) << "Unable to open bitfile: " << bitFile;
		throw runtime_error("Unable to open bitfile.");
	}

	//Copy over the file data to the data vector
	//The default istreambuf_iterator constructor returns the "end-of-stream" iterator.
	vector<UCHAR> fileData((std::istreambuf_iterator<char>(FID)), std::istreambuf_iterator<char>());
	ULONG numBytes = fileData.size();
	FILE_LOG(logDEBUG) << "Read " << numBytes << " bytes from bitfile";

	//Pass of the data to a lower-level function to actually push it to the FPGA
	int bytesProgrammed = handle_.program_FPGA(fileData);

	handle_.select_FPGA_image();

	if (bytesProgrammed == EthernetControl::SUCCESS && expectedVersion != -1) {
		// Read Bit File Version
		int version;
		bool ok = false;
		for (int ct = 0; ct < 20 && !ok; ct++) {
			version =  APS2::read_bitFile_version();
			if (version == expectedVersion) ok = true;
			usleep(1000); // if doesn't match, wait a bit and try again
		}
		if (!ok) return -11;
	}

	return 0;
}

int APS2::read_bitFile_version() {
	// Reads version information from register 0x8006

	uint32_t version;

	//For single FPGA we return that version, for both we return both if the same otherwise error.
	handle_.ReadRegister(FPGA_ADDR_VERSION, version);
	version &= 0x1FF; // First 9 bits hold version
	FILE_LOG(logDEBUG) << "Bitfile version for FPGA is "  << myhex << version;
	
	return version;
}

int APS2::set_sampleRate(const int & freq){
	if (samplingRate_ != freq){
		//Set PLL frequency for each fpga
		APS2::set_PLL_freq(freq);

		samplingRate_ = freq;

		//Test the sync
		return APS2::test_PLL_sync();
	}
	else{
		return 0;
	}
}

int APS2::get_sampleRate() {
	//Pass through to FPGA code
	FILE_LOG(logDEBUG2) << "get_sampleRate";
	int freq1 = APS2::get_PLL_freq();
	return freq1;
}

int APS2::clear_channel_data() {
	FILE_LOG(logINFO) << "Clearing all channel data for APS2 " << deviceID_;
	for (auto & ch : channels_) {
		ch.clear_data();
	}
	// clear waveform length registers
	handle_.WriteRegister(FPGA_ADDR_CHA_WF_LENGTH, 0);
	handle_.WriteRegister(FPGA_ADDR_CHB_WF_LENGTH, 0);
	
	// clear LL length registers
	handle_.WriteRegister(FPGA_ADDR_CHA_LL_LENGTH, 0);
	handle_.WriteRegister(FPGA_ADDR_CHB_LL_LENGTH, 0);
	
	flush();

	return 0;
}

int APS2::load_sequence_file(const string & seqFile){
	/*
	 * Load a sequence file from an H5 file
	 */
	//First open the file
	try {
		FILE_LOG(logINFO) << "Opening sequence file: " << seqFile;
		H5::H5File H5SeqFile(seqFile, H5F_ACC_RDONLY);

		const vector<string> chanStrs = {"chan_1", "chan_2", "chan_3", "chan_4"};
		//For now assume 4 channel data
		//Reset the channel data
		clear_channel_data();
		//TODO: check the channelDataFor attribute
		for(int chanct=0; chanct<4; chanct++){
			//Load the waveform library first
			string chanStr = chanStrs[chanct];
			vector<short> tmpVec = h5array2vector<short>(&H5SeqFile, chanStr + "/waveformLib", H5::PredType::NATIVE_INT16);
			set_waveform(chanct, tmpVec);

			//Check if there is the linklist data and if it is IQ mode style
			H5::Group chanGroup = H5SeqFile.openGroup(chanStr);
			USHORT isLinkListData, isIQMode;
			isLinkListData = h5element2element<USHORT>("isLinkListData", &chanGroup, H5::PredType::NATIVE_UINT16);
			isIQMode = h5element2element<USHORT>("isIQMode", &chanGroup, H5::PredType::NATIVE_UINT16);
			chanGroup.close();

			//Load the linklist data
			if (isLinkListData){
				if (isIQMode){
					channels_[chanct].LLBank_.IQMode = true;
					channels_[chanct].LLBank_.read_state_from_hdf5(H5SeqFile, chanStrs[chanct]+"/linkListData");
					//If the length is less than can fit on the chip then write it to the device
					if (channels_[chanct].LLBank_.length < MAX_LL_LENGTH){
						write_LL_data_IQ(0, 0, channels_[chanct].LLBank_.length, true );
					}

				}
				else{
					channels_[chanct].LLBank_.read_state_from_hdf5(H5SeqFile, chanStrs[chanct]+"/linkListData");
				}
			}
		}
		//Set the mini LL count
		H5::Group rootGroup = H5SeqFile.openGroup("/");
		USHORT miniLLRepeat;
		miniLLRepeat = h5element2element<USHORT>("miniLLRepeat", &rootGroup, H5::PredType::NATIVE_UINT16);
		rootGroup.close();
		set_miniLL_repeat(miniLLRepeat);

		//Close the file
		H5SeqFile.close();
		return 0;
	}
	catch (H5::FileIException & e) {
		return -1;
	}
	return 0;
}

int APS2::set_channel_enabled(const int & dac, const bool & enable){
	return channels_[dac].set_enabled(enable);
}

bool APS2::get_channel_enabled(const int & dac) const{
	return channels_[dac].get_enabled();
}

int APS2::set_channel_offset(const int & dac, const float & offset){
	//Update the waveform in driver
	channels_[dac].set_offset(offset);
	//Write to device if necessary
	if (!channels_[dac].waveform_.empty()){
		write_waveform(dac, channels_[dac].prep_waveform());
	}

	//Update TAZ register
	set_offset_register(dac, channels_[dac].get_offset());

	return 0;
}

float APS2::get_channel_offset(const int & dac) const{
	return channels_[dac].get_offset();
}

int APS2::set_channel_scale(const int & dac, const float & scale){
	channels_[dac].set_scale(scale);
	if (!channels_[dac].waveform_.empty()){
		write_waveform(dac, channels_[dac].prep_waveform());
	}
	return 0;
}

float APS2::get_channel_scale(const int & dac) const{
	return channels_[dac].get_scale();
}

int APS2::set_trigger_source(const TRIGGERSOURCE & triggerSource){

	int returnVal;
	switch (triggerSource){
	case INTERNAL:
		returnVal = FPGA::clear_bit(handle_, FPGA_ADDR_CSR, CSRMSK_CHA_TRIGSRC);
		break;
	case EXTERNAL:
		returnVal = FPGA::set_bit(handle_, FPGA_ADDR_CSR, CSRMSK_CHA_TRIGSRC);
		break;
	default:
		returnVal = -1;
		break;
	}
	return returnVal;
}

TRIGGERSOURCE APS2::get_trigger_source() {
	uint32_t regVal;
	handle_.ReadRegister(FPGA_ADDR_CSR, regVal);
	return TRIGGERSOURCE((regVal & CSRMSK_CHA_TRIGSRC) == CSRMSK_CHA_TRIGSRC ? 1 : 0);
}

int APS2::set_trigger_interval(const double & interval){

	//SM clock is 1/4 of samplingRate so the trigger interval in SM clock periods is
	//note: clockCycles is zero-indexed and has a dead state (so subtract 2)
	int clockCycles = interval*0.25*samplingRate_*1e6 - 2;

	FILE_LOG(logDEBUG) << "Setting trigger interval to " << interval << "s (" << clockCycles << " cycles)";

	//Trigger interval is 32bits wide so have to split up into two 16bit words
	USHORT upperWord = clockCycles >> 16;
	USHORT lowerWord = 0xFFFF  & clockCycles;

	return write(FPGA_ADDR_TRIG_INTERVAL, {upperWord, lowerWord}, false);
}

double APS2::get_trigger_interval() {

	//Trigger interval is 32bits wide so have to split up into two 16bit words reads
	uint32_t upperWord, lowerWord;
	handle_.ReadRegister(FPGA_ADDR_TRIG_INTERVAL,upperWord );
	handle_.ReadRegister(FPGA_ADDR_TRIG_INTERVAL+1, lowerWord);
	
	//Put it back together and covert from clock cycles to time (note: trigger interval is zero indexed and has a dead state)
	return static_cast<double>((upperWord << 16) + lowerWord + 2)/(0.25*samplingRate_*1e6);
}

int APS2::set_miniLL_repeat(const USHORT & miniLLRepeat){
	return handle_.WriteRegister(FPGA_ADDR_LL_REPEAT, miniLLRepeat);
}


int APS2::run() {
	//Depending on how the channels are enabled, trigger the appropriate FPGA's
	vector<bool> channelsEnabled;
	bool allChannels = true;
	for (const auto tmpChannel : channels_){
		channelsEnabled.push_back(tmpChannel.enabled_);
		allChannels &= tmpChannel.enabled_;
	}

	//If we have more LL entries than we can handle then we need to stream
	
	/*
	for (int chanct = 0; chanct < 4; ++chanct) {
		if (channelsEnabled[chanct]){
			if (channels_[chanct].LLBank_.length > MAX_LL_LENGTH && !myBankBouncerThreads_[chanct].isRunning()){
				streaming_ = false;
				//myBankBouncerThreads_[chanct].start();
				while(!streaming_){
					usleep(10000);
				}
			}
		}
	}
	*/

	//Grab a lock to pause the streaming threads while writing to the CSR
	mymutex_->lock();
	FILE_LOG(logDEBUG1) << "Releasing state machine....";
	//If all channels are enabled then trigger together
	if (allChannels) {
		FPGA::set_bit(handle_, FPGA_ADDR_CSR, CSRMSK_CHA_SMRSTN );
	}
	else {
		if (channelsEnabled[0] || channelsEnabled[1]) {
			FPGA::set_bit(handle_, FPGA_ADDR_CSR, CSRMSK_CHA_SMRSTN );
		}
		if (channelsEnabled[2] || channelsEnabled[3]) {
			FPGA::set_bit(handle_, FPGA_ADDR_CSR, CSRMSK_CHA_SMRSTN );
		}
	}
	uint32_t CSR;
	handle_.ReadRegister(0, CSR);
	FILE_LOG(logDEBUG2) << "Current CSR: " << CSR;
	mymutex_->unlock();
	return 0;
}

int APS2::stop() {

	// stop all channels
	
	/*
	for (int chanct = 0; chanct < 4; ++chanct) {
		if (channels_[chanct].LLBank_.length > MAX_LL_LENGTH){
			myBankBouncerThreads_[chanct].stop();
		}
	}
	*/

	//Try to stop in a wait for trigger state by making the trigger interval long
	//This leaves the flip-flops in a known state
	auto curTriggerInt = get_trigger_interval();
	auto curTriggerSource = get_trigger_source();
	set_trigger_interval(1);
	set_trigger_source(INTERNAL);
	usleep(1000);

	//Put the state machines back in reset
	FPGA::clear_bit(handle_, FPGA_ADDR_CSR, CSRMSK_CHA_SMRSTN);
	FPGA::clear_bit(handle_, FPGA_ADDR_CSR, CSRMSK_CHA_SMRSTN);

	// restore trigger state
	set_trigger_interval(curTriggerInt);
	set_trigger_source(curTriggerSource);

	return 0;

}

int APS2::set_run_mode(const int & dac, const RUN_MODE & mode) {
/********************************************************************
 * Description : Sets run mode
 *
 * Inputs :     dac - channel 0-3
 *              mode -  enable link list 1 = LL mode 0 = waveform mode
 *
 * Returns : 0 on success < 0 on failure
 *
********************************************************************/
	int dacModeMask;

	// setup register addressing based on DAC
	switch(dac) {
	  case 0:
	  case 2:
	    dacModeMask = CSRMSK_CHA_OUTMODE;
	    break;
	  case 1:
	  case 3:
	    dacModeMask = CSRMSK_CHB_OUTMODE;
	    break;
	  default:
	    return -2;
	}

	//Set the run mode bit
	FILE_LOG(logINFO) << "Setting Run Mode ==> DAC: " << dac << " Mode: " << mode;
	if (mode) {
	  FPGA::set_bit(handle_, FPGA_ADDR_CSR, dacModeMask);
	} else {
	  FPGA::clear_bit(handle_, FPGA_ADDR_CSR, dacModeMask);
	}

	return 0;
}

int APS2::set_repeat_mode(const int & dac, const bool & mode) {
	/*
	 * set_repeat_mode
	 * dac - channel (0-3)
	 * mode - 1 = one-shot 0 = continuous
	 */
	int dacModeMask;

	// setup register addressing based on DAC
	switch(dac) {
	  case 0:
	  case 2:
	    dacModeMask   = CSRMSK_CHA_REPMODE;
	    break;
	  case 1:
	  case 3:
	    dacModeMask   = CSRMSK_CHB_REPMODE;
	    break;
	  default:
	    return -2;
	}

	//Set or clear the mode bit
	FILE_LOG(logINFO) << "Setting repeat mode ==> DAC: " << dac << " Mode: " << mode;
	if (mode) {
		  FPGA::set_bit(handle_, FPGA_ADDR_CSR, dacModeMask);
	} else {
		  FPGA::clear_bit(handle_, FPGA_ADDR_CSR, dacModeMask);
	}

	return 0;
}

int APS2::set_LLData_IQ(const WordVec & addr, const WordVec & count, const WordVec & trigger1, const WordVec & trigger2, const WordVec & repeat){

	//We store the IQ linklist data in channels 1 and 3
	int dataChan;

	dataChan = 0;
	channels_[dataChan].LLBank_ = LLBank(addr, count, trigger1, trigger2, repeat);

	//If we can fit it on then do so
	if (addr.size() < MAX_LL_LENGTH){
		write_LL_data_IQ(0, 0, addr.size(), true );
	}

	return 0;


}

/*
 *
 * Private Functions
 */

int APS2::write(const unsigned int & addr, const USHORT & data, const bool & queue /* see header for default */){
	//Create the vector and pass through
	return write(addr, vector<USHORT>(1, data), queue);
}

int APS2::write(const unsigned int & addr, const vector<USHORT> & data, const bool & queue /* see header for default */){
	/* APS2::write
	 * fpga = FPAG1, FPGA2, or ALL_FPGAS (for simultaneous writes)
	 * addr = valid memory address to start to write to
	 * data = vector<WORD> data
	 * queue = false - write immediately, true - add write command to output queue
	 */

	//Pack the data
	vector<UCHAR> dataPacket = FPGA::format(addr, data);

	//Update the software checksums
	//Address checksum is defined as lower word of address
	checksum_.address += addr & 0xFFFF;
	for(auto tmpData : data)
		checksum_.data += tmpData;

	//Push into queue or write to FPGA
	auto offsets = FPGA::computeCmdByteOffsets(data.size());
	if (queue) {
		//Calculate offsets of command bytes
		for (auto tmpOffset : offsets){
			offsetQueue_.push_back(tmpOffset + writeQueue_.size());
		}
		for (auto tmpByte : dataPacket){
			writeQueue_.push_back(tmpByte);
		}
	}
	else{
		FPGA::write_block(handle_, dataPacket, offsets);
	}

	return 0;
}



int APS2::flush() {
	// flush write queue to USB interface
	FILE_LOG(logERROR) << "Queue flush needs to be implemented for APS2";
	int bytesWritten = 0;
	//int bytesWritten = FPGA::write_block(handle_, writeQueue_, offsetQueue_);
	FILE_LOG(logDEBUG1) << "Flushed " << bytesWritten << " bytes to device";
	writeQueue_.clear();
	offsetQueue_.clear();
	return bytesWritten;
}


int APS2::setup_PLL() {
	// set the on-board PLL to its default state (two 1.2 GHz outputs, and one 300 MHz output)
	FILE_LOG(logINFO) << "Setting up PLL";

	// Disable DDRs
	int ddrMask = CSRMSK_CHA_DDR | CSRMSK_CHB_DDR;
	FPGA::clear_bit(handle_, FPGA_ADDR_CSR, ddrMask);
	// disable dac FIFOs
	for (int dac = 0; dac < NUM_CHANNELS; dac++)
		disable_DAC_FIFO(dac);

	// Setup modified for 300 MHz FPGA clock rate
	//Setup of a vector of address-data pairs for all the writes we need for the PLL routine
	const vector<AddrData> PLL_Routine = {
		{0x0,  0x99},  // Use SDO, Long instruction mode
		{0x10, 0x7C},  // Enable PLL , set charge pump to 4.8ma
		{0x11, 0x5},  // Set reference divider R to 5 to divide 125 MHz reference to 25 MHz
		{0x14, 0x06},  // Set B counter to 6
		{0x16, 0x5},   // Set P prescaler to 16 and enable B counter (N = P*B = 96 to divide 2400 MHz to 25 MHz)
		{0x17, 0x4},   // Selects readback of N divider on STATUS bit in Status/Control register
		{0x18, 0x60},  // Calibrate VCO with 2 divider, set lock detect count to 255, set high range
		{0x1A, 0x2D},  // Selects readback of PLL Lock status on LOCK bit in Status/Control register
		{0x1C, 0x7},   // Enable differential reference, enable REF1/REF2 power, disable reference switching
		{0xF0, 0x00},  // Enable un-inverted 400mv clock on OUT0
		{0xF1, 0x00},  // Enable un-inverted 400mv clock on OUT1
		{0xF2, 0x00},  // Enable un-inverted 400mv clock on OUT2
		{0xF3, 0x00},  // Enable un-inverted 400mv clock on OUT3
		{0xF4, 0x00},  // Enable un-inverted 400mv clock on OUT4
		{0xF5, 0x00},  // Enable un-inverted 400mv clock on OUT5
		{0x190, 0x00}, // No division on channel 0
		{0x191, 0x80}, // Bypass 0 divider
		{0x193, 0x11}, // (2 high, 2 low = 1.2 GHz / 4 = 300 MHz = Reference 300 MHz)
		{0x196, 0x00}, // No division on channel 2
		{0x197, 0x80}, // Bypass 2 divider
		{0x1E0, 0x0},  // Set VCO post divide to 2
		{0x1E1, 0x2},  // Select VCO as clock source for VCO divider
		{0x232, 0x1},  // Set bit 0 to 1 to simultaneously update all registers with pending writes.
		{0x18, 0x71},  // Initiate Calibration.  Must be followed by Update Registers Command
		{0x232, 0x1},  // Set bit 0 to 1 to simultaneously update all registers with pending writes.
		{0x18, 0x70},  // Clear calibration flag so that next set generates 0 to 1.
		{0x232, 0x1},  // Set bit 0 to 1 to simultaneously update all registers with pending writes.
	};


	// write routine to APS2
	handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, PLL_Routine);

	// enable the oscillator
	if (APS2::reset_status_ctrl() != 1)
		return -1;

	// Enable DDRs
	FPGA::set_bit(handle_, FPGA_ADDR_CSR, ddrMask);

	//Record that sampling rate has been set to 1200
	samplingRate_ = 1200;

	return 0;
}



int APS2::set_PLL_freq(const int & freq) {
	/* APS2::set_PLL_freq
	 * fpga = FPGA1, FPGA2, or ALL_FPGAS
	 * freq = frequency to set in MHz, allowed values are (1200, 600, 300, 200, 100, 50, and 40)
	 */

	ULONG pllCyclesAddr, pllBypassAddr;
	UCHAR pllCyclesVal, pllBypassVal;

	FILE_LOG(logDEBUG) << "Setting PLL FPGA: Freq.: " << freq;


	pllCyclesAddr = FPGA1_PLL_CYCLES_ADDR;
	pllBypassAddr = FPGA1_PLL_BYPASS_ADDR;

	switch(freq) {
//		case 40: pllCyclesVal = 0xEE; break; // 15 high / 15 low (divide by 30)
//		case 50: pllCyclesVal = 0xBB; break;// 12 high / 12 low (divide by 24)
//		case 100: pllCyclesVal = 0x55; break; // 6 high / 6 low (divide by 12)
		case 200: pllCyclesVal = 0x22; break; // 3 high / 3 low (divide by 6)
		case 300: pllCyclesVal = 0x11; break; // 2 high /2 low (divide by 4)
		case 600: pllCyclesVal = 0x00; break; // 1 high / 1 low (divide by 2)
		case 1200: pllCyclesVal = 0x00; break; // value ignored, set bypass below
		default:
			return -2;
	}

	// bypass divider if freq == 1200
	pllBypassVal = (freq==1200) ?  0x80 : 0x00;
	FILE_LOG(logDEBUG2) << "Setting PLL cycles addr: " << myhex << pllCyclesAddr << " val: " << int(pllCyclesVal);
	FILE_LOG(logDEBUG2) << "Setting PLL bypass addr: " << myhex << pllBypassAddr << " val: " << int(pllBypassVal);

	// Disable DDRs
	int ddr_mask = CSRMSK_CHA_DDR | CSRMSK_CHB_DDR;
	FPGA::clear_bit(handle_, FPGA_ADDR_CSR, ddr_mask);
	// disable DAC FIFOs
	for (int dac = 0; dac < NUM_CHANNELS; dac++)
		disable_DAC_FIFO(dac);

	// Disable oscillator by clearing APS2_STATUS_CTRL register
	if (APS2::clear_status_ctrl() != 1) return -4;

	//Setup of a vector of address-data pairs for all the writes we need for the PLL routine
	const vector<AddrData> PLL_Routine = {
		{pllCyclesAddr, pllCyclesVal},
		{pllBypassAddr, pllBypassVal},
		{0x18, 0x71}, // Initiate Calibration.  Must be followed by Update Registers Command
		{0x232, 0x1}, // Set bit 0 to 1 to simultaneously update all registers with pending writes.
		{0x18, 0x70}, // Clear calibration flag so that next set generates 0 to 1.
		{0x232, 0x1} // Set bit 0 to 1 to simultaneously update all registers with pending writes.
	};

	cout << "WritePLLSPI" << endl;

	// write routine to APS2
	handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, PLL_Routine);


	cout << "Enable Oscillator" << endl;

	// Enable Oscillator
	if (APS2::reset_status_ctrl() != 1) return -4;

	// Enable DDRs
	FPGA::set_bit(handle_, FPGA_ADDR_CSR, ddr_mask);
	// Enable DAC FIFOs
	// for (int dac = 0; dac < NUM_CHANNELS; dac++)
	// 	enable_DAC_FIFO(dac);

	return 0;
}



int APS2::test_PLL_sync(const int & numRetries /* see header for default */) {
	/*
		APS2_TestPllSync synchronized the phases of the DAC clocks with the following procedure:
		1) Make sure all PLLs have locked.
		2) Test for sync of 600 MHz clocks from DACs. They must be in sync with each other
	    and in sync with the 300 MHz reference. This has the test signature of the 600 MHz
	    XOR being always low and both 300 MHz XORs are low or high.
			- If either the 600 MHz XOR is high or a 300 MHz XOR is in the middle, disable
			 and re-enable the PLL output to one of the DACs connected to the FPGA. Reset
			 the FPGA PLLs, wait for lock, then loop.
		3) Test channel 0/2 PLL against reference PLL. Reset until in phase.
		4) Test channel 1/3 PLL against reference PLL. Reset until in phase.
		5) Verify that sync worked by testing 0/2 XOR 1/3 (global phase).
	 *
	 * Inputs: device
	 *         fpga (1 or 2)
	 *         numRetries - number of times to restart the test if the global sync test fails (step 5)
	 */

	// Test for DAC clock phase match
	bool inSync, globalSync;
	int xorFlagCnts, a_phase, b_phase;
	int dac02Reset, dac13Reset;

	int pllBit;
	UINT pllEnableAddr, pllEnableAddr2;
	UCHAR writeByte;

	const vector<int> PLL_XOR_TEST = {PLL_02_XOR_BIT, PLL_13_XOR_BIT,PLL_GLOBAL_XOR_BIT};
	const vector<int> CH_PHASE_TESTS = {FPGA_ADDR_A_PHASE, FPGA_ADDR_B_PHASE};
	const vector<int> PLL_LOCK_TEST = {PLL_02_LOCK_BIT, PLL_13_LOCK_BIT, REFERENCE_PLL_LOCK_BIT};
	const vector<int> PLL_RESET = {CSRMSK_CHA_PLLRST, CSRMSK_CHB_PLLRST, 0};

	UINT pllResetBit  = CSRMSK_CHA_PLLRST | CSRMSK_CHB_PLLRST;

	FILE_LOG(logINFO) << "Running channel sync on FPGA ";

	pllEnableAddr = DAC0_ENABLE_ADDR;
	pllEnableAddr2 = DAC1_ENABLE_ADDR;
	
	// Disable DDRs
	int ddr_mask = CSRMSK_CHA_DDR | CSRMSK_CHB_DDR;
	FPGA::clear_bit(handle_, FPGA_ADDR_CSR, ddr_mask);
	// disable DAC FIFOs
	for (int dac = 0; dac < NUM_CHANNELS; dac++)
		disable_DAC_FIFO(dac);

	//A little helper function to wait for the PLL's to lock and reset if necessary
	auto wait_PLL_relock = [this, &pllResetBit](bool resetPLL, const int & regAddress, const vector<int> & pllBits) -> bool {
		FILE_LOG(logDEBUG2) << "wait_PLL_relock";

		bool inSync = false;
		int testct = 0;
		while (!inSync && (testct < 20)){
			FILE_LOG(logDEBUG2) << "Reading PLL status for pllBits with size " << pllBits.size() << " and first bit is " << pllBits[0];
			inSync = (APS2::read_PLL_status(regAddress, pllBits) == 1);
			//If we aren't locked then reset for the next try by clearing the PLL reset bits
			if (resetPLL) {
				FPGA::clear_bit(handle_, FPGA_ADDR_CSR, pllResetBit);
			}
			//Otherwise just wait
			else{
				usleep(1000);
			}
			testct++;
		}
		return inSync;
	};

	// Step 1: test for the PLL's being locked to the reference
	inSync = wait_PLL_relock(true, FPGA_ADDR_PLL_STATUS, PLL_LOCK_TEST);
	if (!inSync) {
		FILE_LOG(logERROR) << "Reference PLL failed to lock";
		return -5;
	}

	inSync = false; globalSync = false;

	//Step 2:
	// start by testing for a 600 MHz XOR always low

	//First a little helper function to update the PLL registers
	auto update_PLL_register = [this] (){
		ULONG address = 0x232;
		UCHAR data = 0x1;
		handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, address, data);
	};

	auto read_DLL_phase = [this] (int addr) {
		// The phase register holds a 9-bit value [0, 511] representing the phase shift.
		// We convert his value to phase in degrees in the range (-180, 180]
		uint32_t regData;
		handle_.ReadRegister(addr, regData);
		double phase = regData;
		if (phase > 256) {
			phase -= 512;
		}
		phase *= 180.0/256.0;
		return phase;
	};

	FILE_LOG(logINFO) << "Testing for DAC clock phase sync";
	//Loop over number of tries
	static const int xorCounts = 20, lowCutoff = 5, lowPhaseCutoff = 45, highPhaseCutoff = 135;
	for (int ct = 0; ct < MAX_PHASE_TEST_CNT; ct++) {
		//Reset the counts
		xorFlagCnts = 0;
		dac02Reset = 0;
		dac13Reset = 0;

		//Take twenty counts of the the xor data
		for(int xorct = 0; xorct < xorCounts; xorct++) {
			uint32_t regData;
			handle_.ReadRegister(FPGA_ADDR_PLL_STATUS, regData);
			pllBit = regData;
			xorFlagCnts += (pllBit >> PLL_GLOBAL_XOR_BIT) & 0x1;
		}

		// read DACA and DACB phases
		a_phase = read_DLL_phase(FPGA_ADDR_A_PHASE);
		b_phase = read_DLL_phase(FPGA_ADDR_B_PHASE);

		FILE_LOG(logDEBUG1) << "DAC A Phase: " << a_phase << ", DAC B Phase: " << b_phase;

		// due to clock skews, need to accept a range of counts as "0" and "1"
		if ( (xorFlagCnts <= lowCutoff ) &&
				(abs(a_phase) < lowPhaseCutoff || abs(a_phase) > highPhaseCutoff) &&
				(abs(b_phase) < lowPhaseCutoff || abs(b_phase) > highPhaseCutoff) ) {
			// 300 MHz clocks on FPGA are either 0 or 180 degrees out of phase and 600 MHz clocks
			// are in phase. Move on.
			FILE_LOG(logDEBUG1) << "DAC clocks in phase with reference, XOR counts : " << xorFlagCnts;
			//Get out of MAX_PHAST_TEST ct loop
			break;
		}
		// TODO: check that we are dealing with the case of in-phase 600 MHz clocks with BOTH 300 MHz clocks 180 out of phase with the reference
		else {
			// 600 MHz clocks out of phase, reset DAC clocks that are 90/270 degrees out of phase with reference
			FILE_LOG(logDEBUG1) << "DAC clocks out of phase; resetting, XOR init: " << xorFlagCnts;
			writeByte = 0x2; //disable clock outputs
			//If ChA is +/-90 degrees out of phase then reset it
			if (abs(a_phase) >= lowPhaseCutoff && abs(a_phase) <= highPhaseCutoff) {
				dac02Reset = 1;
				handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, pllEnableAddr, writeByte );
			}
			//If ChB is +/-90 degrees out of phase then reset it
			if (abs(b_phase) >= lowPhaseCutoff && abs(b_phase) <= highPhaseCutoff) {
				dac13Reset = 1;
				handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, pllEnableAddr2, writeByte);
			}
			//Actually update things
			update_PLL_register();
			writeByte = 0x0; // enable clock outputs
			if (dac02Reset) 
				handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, pllEnableAddr, writeByte );
			if (dac13Reset)
				handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, pllEnableAddr2, writeByte);
			update_PLL_register();

			// reset FPGA PLLs
			FPGA::set_bit(handle_, FPGA_ADDR_CSR, pllResetBit);
			FPGA::clear_bit(handle_, FPGA_ADDR_CSR, pllResetBit);

			// wait for the PLL to relock
			inSync = wait_PLL_relock(false, FPGA_ADDR_PLL_STATUS, PLL_LOCK_TEST);
			if (!inSync) {
				FILE_LOG(logERROR) << "PLLs did not re-sync after reset";
				return -7;
			}
		}
	}

	//Steps 3,4,5
	const vector<string> chStrs = {"A", "B"};
	for (int ch = 0; ch < 2; ch++) {

		FILE_LOG(logDEBUG) << "Testing channel " << chStrs[ch];
		for (int ct = 0; ct < MAX_PHASE_TEST_CNT; ct++) {

			a_phase = read_DLL_phase(CH_PHASE_TESTS[ch]);

			// here we are looking for in-phase clock
			if (abs(a_phase) < lowPhaseCutoff) {
				globalSync = true;
				break; // passed, move on to next channel
			}
			else {
				// PLLs out of sync, reset
				FILE_LOG(logDEBUG1) << "Channel " << chStrs[ch] << " PLL not in sync.. resetting (phase " << a_phase << " )";
				globalSync = false;

				// reset a single channel PLL
				FPGA::set_bit(handle_, FPGA_ADDR_CSR, PLL_RESET[ch]);
				FPGA::clear_bit(handle_, FPGA_ADDR_CSR, PLL_RESET[ch]);

				// wait for lock
				FILE_LOG(logDEBUG2) << "Waiting for relock of PLL " << ch << " by looking at bit " << PLL_LOCK_TEST[ch];
				inSync = wait_PLL_relock(false, FPGA_ADDR_PLL_STATUS, {PLL_LOCK_TEST[ch]});
				if (!inSync) {
					FILE_LOG(logERROR) << "PLL " << chStrs[ch] << " did not re-sync after reset";
					return -10;
				}
			}
		}
	}

	if (!globalSync) { // failed to sync both channels
		if (numRetries > 0) {
			FILE_LOG(logDEBUG) << "Sync failed; retrying.";
			// restart both DAC clocks and try again
			handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, pllEnableAddr, 0x2);  // MAGIC NUMBER
			handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, pllEnableAddr2, 0x2); // MAGIC NUMBER
			update_PLL_register();
			writeByte = 0x0;
			handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, pllEnableAddr, 0x0);  // MAGIC NUMBER
			handle_.WriteSPI(CHIPCONFIG_TARGET_PLL, pllEnableAddr2, 0x0); // MAGIC NUMBER
			update_PLL_register();

			FPGA::set_bit(handle_, FPGA_ADDR_CSR, pllResetBit);
			FPGA::clear_bit(handle_, FPGA_ADDR_CSR, pllResetBit);

			//Try again by recursively calling the same function
			return test_PLL_sync(numRetries - 1);
		} else {
			// we failed, but enable DDRs to get a usable state
			FPGA::set_bit(handle_, FPGA_ADDR_CSR, ddr_mask);
			// enable DAC FIFOs
			//for (int dac = 0; dac < NUM_CHANNELS; dac++)
				//enable_DAC_FIFO(dac);

			FILE_LOG(logERROR) << "Error could not sync PLLs";
			return -9;
		}
	}


	// Enable DDRs
	FPGA::set_bit(handle_, FPGA_ADDR_CSR, ddr_mask);
	// enable DAC FIFOs
	//for (int dac = 0; dac < NUM_CHANNELS; dac++)
		//enable_DAC_FIFO(dac);

	FILE_LOG(logINFO) << "Sync test complete";
	return 0;
}


int APS2::read_PLL_status(const int & regAddr /*check header for default*/, const vector<int> & pllLockBits  /*check header for default*/ ){
	/*
	 * Helper function to read the status of some PLL bit and whether the main PLL is locked.
	 * fpga = FPGA1, FPGA2, or ALL_FPGAS
	 * regAddr = register to poll for PLL sync status (0x8006 or 0xF006)
	 * PllLockBits = vector of register bit locations to query for lock state
	 */
	
 FILE_LOG(logDEBUG2) << "APS2::read_PLL_status";

	int pllStatus = 1;

//	pll_bit = FPGA::read_FPGA(handle_, FPGA_ADDR_SYNC_REGREAD | FPGA_OFF_VERSION, fpga); // latched to USB clock (has version 0x020)
//	pll_bit = FPGA::read_FPGA(handle_, FPGA_ADDR_REGREAD | FPGA_OFF_VERSION, fpga); // latched to 200 MHz PLL (has version 0x010)

	uint32_t pllRegister;
	handle_.ReadRegister(regAddr, pllRegister);

	//Check each of the clocks in series
	for(int tmpBit : pllLockBits){
		pllStatus &= ((pllRegister >> tmpBit) & 0x1);
		FILE_LOG(logDEBUG2) << "FPGA PLL status: " << ((pllRegister >> tmpBit) & 0x1) << " (bit " << tmpBit << " of " << myhex << pllRegister << " )";
	}
	FILE_LOG(logDEBUG1) << "APS2::read_PLL_status = " << pllStatus;
	return pllStatus;
}

int APS2::get_PLL_freq() {
	// Poll APS2 PLL chip to determine current frequency

	uint16_t pll_cycles_addr, pll_bypass_addr;
	uint8_t pll_cycles_val, pll_bypass_val;

	int freq;

	FILE_LOG(logDEBUG2) << "get_PLL_freq";

	pll_cycles_addr = FPGA1_PLL_CYCLES_ADDR;
	pll_bypass_addr = FPGA1_PLL_BYPASS_ADDR;

	handle_.ReadSPI(CHIPCONFIG_TARGET_PLL, pll_cycles_addr, pll_cycles_val);
	handle_.ReadSPI(CHIPCONFIG_TARGET_PLL, pll_bypass_addr, pll_bypass_val);

	FILE_LOG(logDEBUG3) << "pll_cycles_val = " << (int)pll_cycles_val;
	FILE_LOG(logDEBUG3) << "pll_bypass_val = " << (int)pll_bypass_val;

	// select frequency based on pll cycles setting
	// the values here should match the reverse lookup in FGPA::set_PLL_freq

	if (pll_bypass_val == 0x80 && pll_cycles_val == 0x00)
		freq =  1200;
	else {
		switch(pll_cycles_val) {
			case 0xEE: freq = 40;  break;
			case 0xBB: freq = 50;  break;
			case 0x55: freq = 100; break;
			case 0x22: freq = 200; break;
			case 0x11: freq = 300; break;
			case 0x00: freq = 600; break;
			default:
				return -2;
		}
	}

	FILE_LOG(logDEBUG2) << "PLL frequency for FPGA:  Freq: " << freq;

	return freq;
}


int APS2::setup_VCXO() {
	// Write the standard VCXO setup

	FILE_LOG(logINFO) << "Setting up VCX0";

	// Register 00 VCXO value, MS Byte First
	vector<UCHAR> Reg00Bytes = {0x8, 0x60, 0x0, 0x4};

	// Register 01 VCXO value, MS Byte First
	vector<UCHAR> Reg01Bytes = {0x64, 0x91, 0x0, 0x61};

	// ensure the oscillator is disabled before programming
	if (APS2::clear_status_ctrl() != 1)
		return -1;

	handle_.WriteSPI(CHIPCONFIG_TARGET_VCXO, 0, Reg00Bytes);
	handle_.WriteSPI(CHIPCONFIG_TARGET_VCXO, 0, Reg01Bytes);

	return 0;
}

int APS2::setup_DAC(const int & dac) 
/*
 * Description: Aligns the data valid window of the DAC with the output of the FPGA.
 * inputs: dac = 0, 1, 2, or 3
 */
{
	uint8_t data;
	uint8_t SD, MSD, MHD;
	uint8_t edgeMSD, edgeMHD;
	uint8_t interruptAddr, controllerAddr, sdAddr, msdMhdAddr;

	// For DAC SPI writes, we put DAC select in bits 6:5 of address
	interruptAddr = 0x1 | (dac << 5);
	controllerAddr = 0x6 | (dac << 5);
	sdAddr = 0x5 | (dac << 5);
	msdMhdAddr = 0x4 | (dac << 5);

	if (dac < 0 || dac >= NUM_CHANNELS) {
		FILE_LOG(logERROR) << "FPGA::setup_DAC: unknown DAC, " << dac;
		return -1;
	}
	FILE_LOG(logINFO) << "Setting up DAC " << dac;

	// Step 1: calibrate and set the LVDS controller.
	// Ensure that surveilance and auto modes are off
	// get initial states of registers
	
	const vector<APS2::CHIPCONFIG_IO_TARGET> targets = {CHIPCONFIG_TARGET_DAC_0, CHIPCONFIG_TARGET_DAC_1};

	APS2::CHIPCONFIG_IO_TARGET target = targets[dac];

	handle_.ReadSPI(target, interruptAddr, data);
	FILE_LOG(logDEBUG2) <<  "Reg: " << myhex << int(interruptAddr & 0x1F) << " Val: " << int(data & 0xFF);

	handle_.ReadSPI(target, msdMhdAddr, data);
	FILE_LOG(logDEBUG2) <<  "Reg: " << myhex << int(msdMhdAddr & 0x1F) << " Val: " << int(data & 0xFF);
	
	handle_.ReadSPI(target, sdAddr, data);
	FILE_LOG(logDEBUG2) <<  "Reg: " << myhex << int(sdAddr & 0x1F) << " Val: " << int(data & 0xFF);
	
	handle_.ReadSPI(target, controllerAddr, data);
	FILE_LOG(logDEBUG2) <<  "Reg: " << myhex << int(controllerAddr & 0x1F) << " Val: " << int(data & 0xFF);
	
	data = 0;
	handle_.WriteSPI(target, controllerAddr, data);

	// Slide the data valid window left (with MSD) and check for the interrupt
	SD = 0;  //(sample delay nibble, stored in Reg. 5, bits 7:4)
	MSD = 0; //(setup delay nibble, stored in Reg. 4, bits 7:4)
	MHD = 0; //(hold delay nibble,  stored in Reg. 4, bits 3:0)
	data = SD << 4;
	handle_.WriteSPI(target, sdAddr, data);

	for (MSD = 0; MSD < 16; MSD++) {
		FILE_LOG(logDEBUG2) <<  "Setting MSD: " << int(MSD);
		
		data = (MSD << 4) | MHD;
		handle_.WriteSPI(target, msdMhdAddr, data);
		FILE_LOG(logDEBUG2) <<  "Write Reg: " << myhex << int(msdMhdAddr & 0x1F) << " Val: " << int(data & 0xFF);
		
		//FPGA::read_SPI(handle_, APS2_DAC_SPI, msd_mhd_addr, &data);
		//dlog(DEBUG_VERBOSE2, "Read reg 0x%x, value 0x%x\n", msd_mhd_addr & 0x1F, data & 0xFF);
		handle_.ReadSPI(target, sdAddr, data);
		FILE_LOG(logDEBUG2) <<  "Read Reg: " << myhex << int(sdAddr & 0x1F) << " Val: " << int(data & 0xFF);
		
		bool check = data & 1;
		FILE_LOG(logDEBUG2) << "Check: " << check;
		if (!check)
			break;
	}
	edgeMSD = MSD;
	FILE_LOG(logDEBUG) << "Found MSD: " << int(edgeMSD);

	// Clear the MSD, then slide right (with MHD)
	MSD = 0;
	for (MHD = 0; MHD < 16; MHD++) {
		FILE_LOG(logDEBUG2) <<  "Setting MHD: " << int(MHD);
		
		data = (MSD << 4) | MHD;
		handle_.WriteSPI(target, msdMhdAddr, data);
		
		handle_.ReadSPI(target, sdAddr, data);
		FILE_LOG(logDEBUG2) << "Read: " << myhex << int(data & 0xFF);
		bool check = data & 1;
		FILE_LOG(logDEBUG2) << "Check: " << check;
		if (!check)
			break;
	}
	edgeMHD = MHD;
	FILE_LOG(logDEBUG) << "Found MHD = " << int(edgeMHD);
	SD = (edgeMHD - edgeMSD) / 2;
	FILE_LOG(logDEBUG) << "Setting SD = " << int(SD);

	// Clear MSD and MHD
	MHD = 0;
	data = (MSD << 4) | MHD;
	handle_.WriteSPI(target, msdMhdAddr, data);
	// Set the optimal sample delay (SD)
	data = SD << 4;
	handle_.WriteSPI(target, sdAddr, data);

	// AD9376 data sheet advises us to enable surveilance and auto modes, but this
	// has introduced output glitches in limited testing
	// set the filter length, threshold, and enable surveilance mode and auto mode
	/*int filter_length = 12;
	int threshold = 1;
	data = (1 << 7) | (1 << 6) | (filter_length << 2) | (threshold & 0x3);
	FPGA::write_SPI(handle_, APS2_DAC_SPI, controller_addr, &data);
	*/
	
	// turn on SYNC FIFO
//	enable_DAC_FIFO(dac);

	return 0;
}

int APS2::enable_DAC_FIFO(const int & dac) {

	if (dac < 0 || dac >= NUM_CHANNELS) {
		FILE_LOG(logERROR) << "FPGA::setup_DAC: unknown DAC, " << dac;
		return -1;
	}

	uint8_t data;
	uint16_t syncAddr = 0x0 | (dac << 5);
	uint16_t fifoStatusAddr = 0x7 | (dac << 5);
	FILE_LOG(logDEBUG) << "Enabling DAC " << dac << " FIFO";
	const vector<APS2::CHIPCONFIG_IO_TARGET> targets = {CHIPCONFIG_TARGET_DAC_0, CHIPCONFIG_TARGET_DAC_1};
	APS2::CHIPCONFIG_IO_TARGET target = targets[dac];
	// set sync bit (Reg 0, bit 2)
	handle_.ReadSPI(target, syncAddr, data);
	data = data | (1 << 2);
	int status = handle_.WriteSPI(target, syncAddr, data );
	// read back FIFO phase to ensure we are in a safe zone
	handle_.ReadSPI(target, fifoStatusAddr, data);
	// phase (FIFOSTAT) is in bits <6:4>
	FILE_LOG(logDEBUG2) << "Read: " << myhex << int(data & 0xFF);
	FILE_LOG(logDEBUG) << "FIFO phase = " << ((data & 0x70) >> 4);

	return status;
}

int APS2::disable_DAC_FIFO(const int & dac) {
	uint8_t data, mask;
	uint32_t syncAddr = 0x0 | (dac << 5);
	FILE_LOG(logDEBUG1) << "Disable DAC " << dac << " FIFO";
	// clear sync bit
	handle_.ReadSPI(CHIPCONFIG_TARGET_PLL, syncAddr, data);
	mask = (0x1 << 2);
	return handle_.WriteSPI( CHIPCONFIG_TARGET_PLL, syncAddr, data & ~mask );
}

int APS2::set_offset_register(const int & dac, const float & offset) {
	/* APS2::set_offset_register
	 * Write the zero register for the associated channel
	 * offset - offset in normalized full range (-1, 1)
	 */

	uint32_t zeroRegisterAddr;
	WORD scaledOffset;

	switch (dac) {
		case 0:
		case 2:
			zeroRegisterAddr = FPGA_ADDR_CHA_ZERO;
			break;
		case 1:
			// fall through
		case 3:
			zeroRegisterAddr = FPGA_ADDR_CHB_ZERO;
			break;
		default:
			return -2;
	}

	scaledOffset = WORD(offset * MAX_WF_AMP);
	FILE_LOG(logINFO) << "Setting DAC " << dac << "  zero register to " << scaledOffset;

	handle_.WriteRegister(zeroRegisterAddr, scaledOffset);

	return 0;
}

int APS2::write_waveform(const int & dac, const vector<short> & wfData) {
	/*Write waveform data to FPGA memory
	 * dac = channel (0-3)
	 * wfData = signed short waveform data
	 */

	uint32_t tmpData, wfLength;
	uint32_t sizeReg, startAddr;
	//We assume the Channel object has properly formated the waveform
	// setup register addressing based on DAC
	switch(dac) {
		case 0:
		case 2:
			sizeReg   = FPGA_ADDR_CHA_WF_LENGTH;
			startAddr =  FPGA_BANKSEL_WF_CHA;
			break;
		case 1:
		case 3:
			sizeReg   = FPGA_ADDR_CHB_WF_LENGTH;
			startAddr =  FPGA_BANKSEL_WF_CHB;
			break;
		default:
			return -2;
	}

	//Waveform length used by FPGA must be an integer multiple of WF_MODULUS and is 0 counted
	wfLength = wfData.size() / WF_MODULUS - 1;
	FILE_LOG(logINFO) << "Loading Waveform length " << wfData.size() << " (FPGA count = " << wfLength << " ) into DAC " << dac;

	//Write the waveform parameters
	handle_.WriteRegister(sizeReg, wfLength);

	if (FILELog::ReportingLevel() >= logDEBUG2) {
		//Double check it took
		handle_.ReadRegister(sizeReg, tmpData);
		FILE_LOG(logDEBUG2) << "Size set to: " << tmpData;
		FILE_LOG(logDEBUG2) << "Loading waveform at " << myhex << startAddr;
	}

	//Reset the checksums
	if (FILELog::ReportingLevel() >= logDEBUG) {
		reset_checksums();
	}

	//Format the data and add to write queue
	write(startAddr, vector<USHORT>(wfData.begin(), wfData.end()), true);
	flush();

	//Verify the checksums
	if (FILELog::ReportingLevel() >= logDEBUG) {
		if (!verify_checksums()){
			FILE_LOG(logERROR) << "Checksums didn't match after writing waveform data";
			return -2;
		}
	}
	return 0;
}


int APS2::write_LL_data_IQ(const ULONG & startAddr, const size_t & startIdx, const size_t & stopIdx, const bool & writeLengthFlag ){

	//We store the IQ linklist data in channels 1 and 3
	int dataChan;
	dataChan = 0;
	
	FILE_LOG(logDEBUG) << "Writing LL Data with Start Addr=" << startAddr << "; startIdx=" << startIdx << "; stopIdx=" << stopIdx;
	size_t entriesToWrite;
	if (stopIdx > startIdx){
		entriesToWrite = stopIdx-startIdx;
	}
	else{
		// must be wrapping around the end, compute the modular distance
		entriesToWrite = mymod(static_cast<int>(stopIdx)-static_cast<int>(startIdx), static_cast<int>(channels_[dataChan].LLBank_.length));
	}

	FILE_LOG(logDEBUG1) << "Writing LL Data for Channel: " << dataChan << "; Length: " << entriesToWrite;

	WordVec writeData;

	//Sort out whether we'll have to wrap around the top of the memory
	if ( (startAddr+entriesToWrite) > MAX_LL_LENGTH){
		//Pull out the first segment
		size_t tmpStopIdx = ((MAX_LL_LENGTH-startAddr) + startIdx)%channels_[dataChan].LLBank_.length;
		writeData = channels_[dataChan].LLBank_.get_packed_data(startIdx, tmpStopIdx);
		//queue it
		write(FPGA_BANKSEL_LL_CHA | startAddr, writeData, true);
		//the second segment is written to the top of the memory (startAddr = 0)
		writeData = channels_[dataChan].LLBank_.get_packed_data(tmpStopIdx, stopIdx);
		write(FPGA_BANKSEL_LL_CHA | 0, writeData, true);
	}
	else{
		writeData = channels_[dataChan].LLBank_.get_packed_data(startIdx, stopIdx);
		write(FPGA_BANKSEL_LL_CHA | startAddr, writeData, true);
	}

	//If necessary write the LL length register
	if (writeLengthFlag){
		FILE_LOG(logDEBUG2) << "Writing Link List Length: " << myhex << stopIdx << " at address: " << FPGA_ADDR_CHA_LL_LENGTH;
		write(FPGA_ADDR_CHA_LL_LENGTH, stopIdx-1, true);
	}

	//Flush the queue to the device
	flush();
	return 0;
}

//int APS2::write_LL_data(const int & dac, const int & bankNum, const int & targetBank) {
	/*
	 * write_LL_data
	 * dac = channel (0-3)
	 * bankNum = LL bank number to load (having previously been stored in the driver with add_LL_bank()
	 * targetBank = where to load the bank (0 = bank A, 1 = bank B)
	 */
	//TODO: make work
	/*

	int startAddr;
	int sizeReg;

	FPGASELECT fpga = dac2fpga(dac);
	if (fpga == INVALID_FPGA) {
		return -1;
	}

	// setup register addressing based on DAC
	switch(dac) {
		case 0:
		case 2:
			if (targetBank == 0) {
				startAddr = FPGA_ADDR_CHA_LL_A_WRITE;
				sizeReg = FPGA_OFF_CHA_LL_A_CTRL;
			} else {
				startAddr = FPGA_ADDR_CHA_LL_B_WRITE;
				sizeReg = FPGA_OFF_CHA_LL_B_CTRL;
			}
			break;
		case 1:
		case 3:
			if (targetBank == 0) {
				startAddr    = FPGA_ADDR_CHB_LL_A_WRITE;
				sizeReg = FPGA_OFF_CHB_LL_A_CTRL;
			} else {
				startAddr = FPGA_ADDR_CHB_LL_B_WRITE;
				sizeReg = FPGA_OFF_CHB_LL_B_CTRL;
			}
			break;
		default:
			return -2;
	}

	size_t bankLength = channels_[dac].banks_[bankNum].length;

	if ( int(bankLength) > MAX_LL_LENGTH)  {
		return -3;
	}
	FILE_LOG(logINFO) << "Loading LinkList length " << bankLength << " into DAC " << dac << " bank " << bankNum << " targetBank " << targetBank;

	//Set the link list size
	int lastElementOffset = (targetBank==0) ? (bankLength-1) : (bankLength-1) + MAX_LL_LENGTH ;

	FILE_LOG(logDEBUG2) << "Writing Link List Control Reg: " << myhex << sizeReg << " = " << lastElementOffset;

	// clear checksums
	if (FILELog::ReportingLevel() >= logDEBUG) {
		reset_checksums(fpga);
	}

	// write control reg
	write(fpga, sizeReg, lastElementOffset, true);

	//Write the LL data and flush to device
	write(fpga, startAddr, channels_[dac].banks_[bankNum].get_packed_data(), true);
	flush();

	// verify the checksum if we are in sufficient debug mode
	if (FILELog::ReportingLevel() >= logDEBUG) {
		if (!verify_checksums(fpga)){
			FILE_LOG(logERROR) << "Checksums didn't match after writing LL data";
			return -2;
		}
	}
	return 0;

}
	*/

int APS2::read_LL_addr(){
	/*
	 * Read the currently playing LL address
	 */
	return handle_.ReadRegister(FPGA_ADDR_CHA_LL_CURADDR);
}

int APS2::read_LL_addr(const int & dac){
	/*
	 * Read the currently playing LL address
	 */
	return handle_.ReadRegister(FPGA_ADDR_CHA_LL_CURADDR);
}


int APS2::read_miniLL_startAddr(){
	/*
	 * Read the start of the currently playing miniLL
	 */
	return handle_.ReadRegister(FPGA_ADDR_CHA_MINILLSTART);
}

int APS2::save_state_file(string & stateFile){

	if (stateFile.length() == 0) {
		stateFile += "cache_" + deviceSerial_ + ".h5";
	}

	FILE_LOG(logDEBUG) << "Writing State For Device: " << deviceSerial_ << " to hdf5 file: " << stateFile;
	H5::H5File H5StateFile(stateFile, H5F_ACC_TRUNC);
	string rootStr = "";
	write_state_to_hdf5(H5StateFile, rootStr);
	//Close the file
	H5StateFile.close();
	return 0;
}

int APS2::read_state_file(string & stateFile){

	if (stateFile.length() == 0) {
		stateFile += "cache_" + deviceSerial_ + ".h5";
	}

	FILE_LOG(logDEBUG) << "Reading State For Device: " << deviceSerial_ << " from hdf5 file: " << stateFile;
	H5::H5File H5StateFile(stateFile, H5F_ACC_RDONLY);
	string rootStr = "";
	read_state_from_hdf5(H5StateFile, rootStr);
	//Close the file
	H5StateFile.close();
	return 0;
}

int APS2::write_state_to_hdf5(H5::H5File & H5StateFile, const string & rootStr){
	std::ostringstream tmpStream;
	//For now assume 4 channel data
	for(int chanct=0; chanct<4; chanct++){
		tmpStream.str("");
		tmpStream << rootStr << "/chan_" << chanct+1;
		FILE_LOG(logDEBUG) << "Writing State For Channel " << chanct + 1 << " to hdf5 file";
		FILE_LOG(logDEBUG) << "Creating Group: " << tmpStream.str();
		H5::Group tmpGroup = H5StateFile.createGroup(tmpStream.str());
		tmpGroup.close();
		channels_[chanct].write_state_to_hdf5(H5StateFile,tmpStream.str());
	}
	return 0;
}

int APS2::read_state_from_hdf5(H5::H5File & H5StateFile, const string & rootStr){
	//For now assume 4 channel data
	std::ostringstream tmpStream;
	for(int chanct=0; chanct<4; chanct++){
		tmpStream.str("");
		tmpStream << rootStr << "/chan_" << chanct+1;
		FILE_LOG(logDEBUG) << "Reading State For Channel " << chanct + 1<< " from hdf5 file";
		channels_[chanct].read_state_from_hdf5(H5StateFile,tmpStream.str());
	}
	return 0;
}







