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
#include <random>
#include <tuple>
#include <algorithm>

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
	void sampleFeatureFerns(int w, int h) 
	{
		std::uniform_int_distribution<int> wDist(0, w - 1);
		std::uniform_int_distribution<int> hDist(0, h - 1);

		for (int f = 0; f < numFerns; f++) {
			for (int d = 0; d < fernSize; d++) {
				features[f*fernSize + d] = std::make_tuple(wDist(gen), hDist(gen), wDist(gen), hDist(gen));
			}
		}
		probs = std::vector<float>(twoFernSize*(numClasses)*(numFerns), 1);
		counts = std::vector<float>((numClasses), (1 << (fernSize)));
	}
	void sampleOneFern(int w, int h)
	{
		std::uniform_int_distribution<int> wDist(0, w - 1);
		std::uniform_int_distribution<int> hDist(0, h - 1);
		std::uniform_int_distribution<int> fDist(0, numFerns - 1);
		auto f = fDist(gen);
		for (int d = 0; d < fernSize; d++) {
			features[f*fernSize + d] = std::make_tuple(wDist(gen), hDist(gen), wDist(gen), hDist(gen));
		}

		probs = std::vector<float>(twoFernSize*(numClasses)*(numFerns), 1);
		counts = std::vector<float>((numClasses), (1 << (fernSize)));
	}
	void sampleOneFeature(int w, int h)
	{
		std::uniform_int_distribution<int> wDist(0, w - 1);
		std::uniform_int_distribution<int> hDist(0, h - 1);
		std::uniform_int_distribution<int> fDist(0, numFerns - 1);
		std::uniform_int_distribution<int> ftDist(0, fernSize - 1);
		std::bernoulli_distribution ft(0.5);

		auto f = fDist(gen);
		auto d = ftDist(gen);
		auto which = ft(gen);
		auto curr = features[f*fernSize + d];
		features[f*fernSize + d] = which ? std::make_tuple(std::get<0>(curr), std::get<1>(curr), wDist(gen), hDist(gen)) : std::make_tuple(wDist(gen), hDist(gen), std::get<2>(curr), std::get<3>(curr));

		probs = std::vector<float>(twoFernSize*(numClasses)*(numFerns), 1);
		counts = std::vector<float>((numClasses), (1 << (fernSize)));
	}
	int getHash(const img::Img<uint8_t> & img, const int fern) {
		int hash = 0;
		auto ip = img.data.get();
		for (int l = 0; l < fernSize; l++) {
			auto feature = features[fern*fernSize + l];
			auto p1x = std::get<0>(feature);
			auto p1y = std::get<1>(feature);
			auto p2x = std::get<2>(feature);
			auto p2y = std::get<3>(feature);
			int bit = (ip[img.width*p1y + p1x] > ip[img.width*p2y + p2x]) ? 1 : 0;
			hash |= (bit << l);
		}
		return hash;
	}
	void train(const img::Img<uint8_t> & img, const uint8_t label) {
		for (int f = 0; f < numFerns; f++) {
			auto hash = getHash(img, f);
			probs[f*(numClasses*twoFernSize) + label*twoFernSize + hash]++;
		}
		counts[label]++;

	}
	void sampleNewFernEntropy(int w, int h) {
		std::vector<float> fernClassProbs(numFerns*numClasses, 0); //tem
		std::vector<float> fernSumProbs(numFerns, 0); //nrmzs
		std::vector<float> fernSumInfo(numFerns*numClasses, 0); //nrmzs

		std::vector<float> classProbs(numClasses, 0);

		for (int f = 0; f < numFerns; f++) {
			float sum = 0;
			float ent = 0;
			for (int c = 0; c < numClasses; c++) {
				for (int l = 0; l < twoFernSize; l++) {
					sum +=  static_cast<float>(probs[f*(numClasses*twoFernSize) + c*twoFernSize + l]);
				}
			}
			for (int c = 0; c < numClasses; c++) {
				for (int l = 0; l < twoFernSize; l++) {
					auto p = static_cast<float>(probs[f*(numClasses*twoFernSize) + c*twoFernSize + l]) / sum;
					ent -= p * logf(p);
				}
			}
			fernSumProbs[f] = ent;
			/*
			for (int c = 0; c < numClasses; c++) {
				float ent = 0;
				for (int l = 0; l < twoFernSize; l++) {
					auto p = static_cast<float>(probs[f*(numClasses*twoFernSize) + c*twoFernSize + l]) / sum;
					ent -= p * logf(p);
				}
				fernSumInfo[f*numClasses + c] = ent;
			}*/
			//for (int c = 0; c < numClasses; c++) {
			//	float ent = 0;

			//	for (int l = 0; l < twoFernSize; l++) {
			//		auto p = static_cast<float>(probs[f*(numClasses*twoFernSize) + c*twoFernSize + l])/counts[c];
			//		ent -= p * logf(p);
			//	}
			//	fernClassProbs[f*numClasses + c] = ent;
			//}
		}
		int max_ent_fern = std::min_element(fernSumProbs.begin(), fernSumProbs.end()) - fernSumProbs.begin();
		std::uniform_int_distribution<int> wDist(0, w - 1);
		std::uniform_int_distribution<int> hDist(0, h - 1);
		for (int d = 0; d < fernSize; d++) {
			features[max_ent_fern*fernSize + d] = std::make_tuple(wDist(gen), hDist(gen), wDist(gen), hDist(gen));
		}

		probs = std::vector<float>(twoFernSize*(numClasses)*(numFerns), 1);
		counts = std::vector<float>((numClasses), (1 << (fernSize)));
		//int max_prob_class = std::max_element(classProbs.begin(), classProbs.end()) - classProbs.begin();
		//return max_prob_class;
	}
	//void finishTraining() 
	uint8_t predict(const img::Img<uint8_t> & img) {
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
				prob *= fernClassProbs[f*numClasses + c];
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
	std::vector<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>> features; // fernSize x numFerns. i(0,1) > i(2,3)

	int fernSize, numFerns, numClasses, twoFernSize;

};

int main(int argc, char * argv[])
{
	auto train_img = readMNISTImg("train-images.idx3-ubyte");
	auto train_lbl = readMNISTLabel("train-labels.idx1-ubyte");
	auto test_img = readMNISTImg("t10k-images.idx3-ubyte");
	auto test_lbl = readMNISTLabel("t10k-labels.idx1-ubyte");
	FernClassifier fc(10, 20, 10);
	fc.sampleFeatureFerns(train_img[0].width, train_img[0].height);

	if (false) {
		std::ifstream is("97_out_7.json", std::ios::binary);
		cereal::JSONInputArchive archive(is);
		archive(fc);
	}
	FernClassifier bestFC = fc;
	float bestAcc = 0.0;
	float pAcc = bestAcc;
	int iter = 0;
	int iterations = 15000;
	float startTmp = 1.0;
	float minTmp = 0.00001;
	float tmpFactor = 0.99;
	float temp = startTmp;
	std::cout.precision(3);

	while (true) {
		fc = bestFC;
		fc.sampleNewFernEntropy(train_img[0].width, train_img[0].height);

		//fc.sampleOneFern(train_img[0].width, train_img[0].height);
		for (int i = 0; i < train_img.size(); i++) {
			fc.train(train_img[i], train_lbl[i]);
		}
		int predictQuality[10 * 10] = {};
		float correct = 0;
		for (int i = 0; i < test_img.size(); i++) {
			auto res = fc.predict(test_img[i]);
			predictQuality[10 * res + test_lbl[i]]++;
			if (test_lbl[i] == res)
				correct++;
		}
		float meanAccuracy = correct / test_img.size();
		//fc.sampleNewFernEntropy(train_img[0].width, train_img[0].height);

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

		std::cout << meanAccuracy << "\t" << iter << "\t" << temp << std::endl;

		iter++;
		if (iter >= iterations) {
			iter = 0;
			fc.sampleFeatureFerns(train_img[0].width, train_img[0].height);
			bestFC = fc;
			pAcc = 0.0;
			temp = startTmp;
		}
	}
	return 0;
}