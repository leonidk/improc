#include "image_io.h"
#include "image_filter.h"

#ifdef _WIN32 
#include <intrin.h>
#define bSwap _byteswap_ulong
#else
#define bSwap __builtin_bswap32
#endif
#include <vector>
#include <fstream>
#include <iostream>
#include <random>
#include <tuple>
#include <algorithm>
#include <cstring>
std::vector<img::Img<uint8_t>> readMNISTImg(const std::string &fileName)
{
	std::vector<img::Img<uint8_t>> imgs;
	std::ifstream file(fileName,std::ios::binary);
	if (file.is_open())
	{
		uint32_t magic_number = 0;
		uint32_t number_of_images = 0;
		uint32_t n_rows = 0;
		uint32_t n_cols = 0;
		file.read((char*)&magic_number, sizeof(magic_number));
		magic_number = bSwap(magic_number);
		file.read((char*)&number_of_images, sizeof(number_of_images));
		number_of_images = bSwap(number_of_images);
		file.read((char*)&n_rows, sizeof(n_rows));
		n_rows = bSwap(n_rows);
		file.read((char*)&n_cols, sizeof(n_cols));
		n_cols = bSwap(n_cols);
		for (int i = 0; i<number_of_images; ++i)
		{
			img::Img<uint8_t> img(n_cols,n_rows);
			file.read((char*)img.data.get(), n_rows*n_cols);
			imgs.push_back(img);
		}
	}

	return imgs;
}

std::vector<uint8_t> readMNISTLabel(const std::string &fileName)
{
	std::vector<uint8_t> labels;
	std::ifstream file(fileName, std::ios::binary);
	if (file.is_open())
	{
		uint32_t magic_number = 0;
		uint32_t n_items = 0;
		file.read((char*)&magic_number, sizeof(magic_number));
		magic_number = bSwap(magic_number);
		file.read((char*)&n_items, sizeof(n_items));
		n_items = bSwap(n_items);
		labels.resize(n_items);
		file.read((char*)labels.data(), n_items);
	}
	return labels;
}

std::vector<uint8_t> readShapeNetLabel(const std::string &fileName)
{
    std::ifstream filelist(fileName);
	std::vector<uint8_t> labels;
	for (std::string line; std::getline(filelist, line); ) {
        std::ifstream file(line, std::ios::binary);
        if (file.is_open())
        {
            while (! file.eof() ) {
                uint8_t label;
                file.read((char*)&label, sizeof(uint8_t));
                labels.push_back(label);
            }
        } else {
            std::cout << "failed to open file\t" << line <<"\t"<< strerror(errno) << std::endl;
        }
    }
	return labels;
}
struct grid30shape {
    uint8_t data[30*30*30];
};
std::vector<grid30shape> readShapeNetData(const std::string &fileName)
{
    std::ifstream filelist(fileName);
	std::vector<grid30shape> data;
	for (std::string line; std::getline(filelist, line); ) {
        std::ifstream file(line, std::ios::binary);
        if (file.is_open())
        {
            while (! file.eof() ) {
                grid30shape datum;
                file.read((char*)&datum, sizeof(grid30shape));
                data.push_back(datum);
            }
        } else {
            std::cout << "failed to open file\t" << line <<"\t"<< strerror(errno) << std::endl;
        }
    }
	return data;
}
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

#include <cereal/types/vector.hpp>
#include <cereal/types/tuple.hpp>
std::random_device rd;
std::mt19937 gen(rd());

class FernClassifier {
public:
	FernClassifier(int fernSize, int numFerns, int numClasses)
		: fernSize(fernSize), numFerns(numFerns), numClasses(numClasses), twoFernSize(1 << (fernSize)),
		probs((1 << (fernSize))*(numClasses)*(numFerns), 1), 
		counts((numClasses), (1 << (fernSize))),
		features(fernSize*numFerns)
	{
	}
	void sampleFeatureFerns() 
	{
		std::uniform_int_distribution<int> wDist(0, 30 - 1);
		std::uniform_int_distribution<int> hDist(0, 30 - 1);
		std::uniform_int_distribution<int> dDist(0, 30 - 1);

		for (int f = 0; f < numFerns; f++) {
			for (int d = 0; d < fernSize; d++) {
				features[f*fernSize + d] = std::make_tuple(wDist(gen), hDist(gen), dDist(gen),wDist(gen), hDist(gen), dDist(gen));
			}
		}
		probs = std::vector<float>(twoFernSize*(numClasses)*(numFerns), 1);
		counts = std::vector<float>((numClasses), (1 << (fernSize)));
	}
	void sampleOneFern()
	{
		std::uniform_int_distribution<int> wDist(0, 30 - 1);
		std::uniform_int_distribution<int> hDist(0, 30 - 1);
		std::uniform_int_distribution<int> dDist(0, 30 - 1);
		std::uniform_int_distribution<int> fDist(0, numFerns - 1);
		auto f = fDist(gen);
		for (int d = 0; d < fernSize; d++) {
			features[f*fernSize + d] = std::make_tuple(wDist(gen), hDist(gen), dDist(gen),wDist(gen), hDist(gen), dDist(gen));
		}

		probs = std::vector<float>(twoFernSize*(numClasses)*(numFerns), 1);
		counts = std::vector<float>((numClasses), (1 << (fernSize)));
	}
	void sampleOneFeature()
	{
		std::uniform_int_distribution<int> wDist(0, 30 - 1);
		std::uniform_int_distribution<int> hDist(0, 30 - 1);
		std::uniform_int_distribution<int> dDist(0, 30 - 1);
		std::uniform_int_distribution<int> fDist(0, numFerns - 1);
		std::uniform_int_distribution<int> ftDist(0, fernSize - 1);
		std::bernoulli_distribution ft(0.5);

		auto f = fDist(gen);
		auto d = ftDist(gen);
		auto which = ft(gen);
		auto curr = features[f*fernSize + d];
		features[f*fernSize + d] = which ? 
            std::make_tuple((uint8_t)std::get<0>(curr), (uint8_t)std::get<1>(curr),(uint8_t)std::get<2>(curr),
                            (uint8_t)wDist(gen), (uint8_t)hDist(gen),(uint8_t)dDist(gen)) : 
            std::make_tuple((uint8_t)wDist(gen), (uint8_t)hDist(gen), (uint8_t)dDist(gen),
                            (uint8_t)std::get<3>(curr), (uint8_t)std::get<4>(curr),(uint8_t)std::get<5>(curr));

		probs = std::vector<float>(twoFernSize*(numClasses)*(numFerns), 1);
		counts = std::vector<float>((numClasses), (1 << (fernSize)));
	}
	int getHash(const grid30shape & img, const int fern) {
		int hash = 0;
		for (int l = 0; l < fernSize; l++) {
			auto feature = features[fern*fernSize + l];
			auto p1x = std::get<0>(feature);
			auto p1y = std::get<1>(feature);
			auto p1z = std::get<2>(feature);
			auto p2x = std::get<3>(feature);
			auto p2y = std::get<4>(feature);
			auto p2z = std::get<5>(feature);
			int bit = (img.data[(30*30)*p1z + 30*p1y + p1x] > img.data[(30*30)*p2z + 30*p2y + p2x]) ? 1 : 0;
			hash |= (bit << l);
		}
		return hash;
	}
	void train(const grid30shape & img, const uint8_t label) {
		for (int f = 0; f < numFerns; f++) {
			auto hash = getHash(img, f);
			probs[f*(numClasses*twoFernSize) + label*twoFernSize + hash]++;
		}
		counts[label]++;

	}
	//void finishTraining() 
	uint8_t predict(const grid30shape & img) {
		std::vector<float> fernClassProbs(numFerns*numClasses, 0); //tem
		std::vector<float> fernSumProbs(numFerns, 0); //nrmzs

		std::vector<float> classProbs(numClasses, 0);

		for (int f = 0; f < numFerns; f++) {
			auto hash = getHash(img, f);
			for (int c = 0; c < numClasses; c++) {
				auto probF_C = static_cast<float>(probs[f*(numClasses*twoFernSize) + c*twoFernSize + hash]) / static_cast<float>(counts[c]);
				fernClassProbs[f*numClasses+c] = probF_C;
				fernSumProbs[f] += probF_C;
			}
		}
		for (int f = 0; f < numFerns; f++) {
			for (int c = 0; c < numClasses; c++) {
				fernClassProbs[f*numClasses + c] /= fernSumProbs[f];
			}
		}
		for (int c = 0; c < numClasses; c++) {
			auto prob = 1.0;
			for (int f = 0; f < numFerns; f++) {
				prob += logf(fernClassProbs[f*numClasses + c]);
			}
			classProbs[c] = prob;
		}
		int max_prob_class = std::max_element(classProbs.begin(), classProbs.end()) - classProbs.begin();
		return max_prob_class;
	}
	template <class Archive>
	void serialize(Archive & ar)
	{
		ar(fernSize);
		ar(numFerns);
		ar(numClasses);
		ar(twoFernSize);

		ar(probs);
		ar(counts);
		ar(features);
	}
private:
	std::vector<float> probs; // 2^fernSize x numClasses x numFerns
	std::vector<float> counts; // numClasses
	std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t,uint8_t,uint8_t>> features; // fernSize x numFerns. i(0,1) > i(2,3)

	int fernSize, numFerns, numClasses, twoFernSize;

};
int main(int argc, char * argv[])
{
    auto train_lbl_sn = readShapeNetLabel("train_lbl.txt");
    std::cout << train_lbl_sn.size() << std::endl;
    auto train_raw_sn = readShapeNetData("train_raw.txt");
    std::cout << train_raw_sn.size() << std::endl;
    //for(int fernum = 19; fernum < 40; fernum++) {
    //    for(int fernd = 8; fernd < 10; fernd++) {
	FernClassifier fc(9, 29, 40);
	fc.sampleFeatureFerns();
	/*{
		//83_out.json
		std::ifstream is("97_out_7.json", std::ios::binary);
		cereal::JSONInputArchive archive(is);
		archive(fc);
	}*/
	FernClassifier bestFC = fc;
	float bestAcc =0.4;
	float pAcc = bestAcc;
	int iter = 0;
	int iterations = 15000;
	float startTmp = 0.1;
	float minTmp = 0.00001;
	float tmpFactor = 0.99;
	float temp = startTmp;
	std::cout.precision(3);

	while (true) {
		fc = bestFC;
        //fc.sampleFeatureFerns();
		fc.sampleOneFern();
		//fc.sampleOneFeature();
		for (int i = 0; i < train_raw_sn.size(); i++) {
			fc.train(train_raw_sn[i], train_lbl_sn[i]);
		}
		int predictQuality[10 * 10] = {};
		float correct = 0;
        for (int i = 0; i < train_lbl_sn.size(); i+=1) {
			auto res = fc.predict(train_raw_sn[i]);
			predictQuality[10 * res + train_lbl_sn[i]]++;
			if (train_lbl_sn[i] == res)
				correct++;
		}
		float meanAccuracy = correct / train_lbl_sn.size();
		//std::cout << meanAccuracy << std::endl;
		auto sqr = [](float x) { return x*x; };
		auto CONST = 0.1f;
		//temp = CONST*sqr(((float)(iterations - iter)) / ((float)iterations));
		float cost = exp(-(pAcc - meanAccuracy) / temp);
		std::uniform_real_distribution<float> cDist(0, 1.0f);
		temp = std::max(tmpFactor*temp, minTmp);
		float sample = cDist(gen);

		if (meanAccuracy > bestAcc)
		{
			bestFC = fc;
			bestAcc = meanAccuracy;
			pAcc = meanAccuracy;
			std::cout << "NewBest" << std::endl;
			std::ofstream os(std::to_string(std::round(meanAccuracy*100)).substr(0,2) + "_out.json", std::ios::binary);
			cereal::JSONOutputArchive archive(os);
			archive(fc);
		}
		else if (meanAccuracy > pAcc) {
			bestFC = fc;
			pAcc = meanAccuracy;
			std::cout << "acceptedBetter" << std::endl;
		}
		else if (cost > sample) {
			bestFC = fc;
			pAcc = meanAccuracy;
			std::cout << "accepted" << std::endl;
		}
        //std::cout << fernum <<'\t' <<  fernd << std::endl;
		std::cout << meanAccuracy <<"\t" << cost << "\t" << sample << "\t" << iter << "\t" << temp << std::endl;

		iter++;
		if (iter >= iterations) {
			iter = 0;
			fc.sampleFeatureFerns();
			bestFC = fc;
			pAcc = 0.0;
			temp = startTmp;
		}
	}
  //      }
  //  }
	return 0;
}
