#include "Util/SymSpell/SymSpell.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <limits>
#include <cmath>
#include "Util/StringUtil.h"

extern std::vector<std::pair<std::string, int64_t>> BuildInWords;

SymSpell::SymSpell()
	: _prefixLength(5),
	  _maxDictionaryWordLength(0),
	  _compactMask((std::numeric_limits<uint32_t>::max() >> 8) << 2)
{
}

SymSpell::~SymSpell()
{
}

bool SymSpell::LoadWordDictionary(std::string path)
{
	std::fstream fin(path, std::ios::in);
	if (!fin.is_open())
	{
		return false;
	}

	std::stringstream s;
	s << fin.rdbuf();
	auto dictionarySource = s.str();

	auto lines = StringUtil::Split(dictionarySource, "\n");
	try
	{
		for (auto& line : lines)
		{
			auto tokens = StringUtil::Split(line, " ");
			if (tokens.size() < 2)
			{
				continue;
			}

			int64_t count = std::stoll(std::string(tokens[1]));

			CreateDictionaryEntry(tokens[0], count);
		}
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return false;
	}

	return true;
}

bool SymSpell::LoadBuildInDictionary()
{
	return true;
}

bool SymSpell::CreateDictionaryEntry(std::string_view key, int64_t count)
{
	if (count <= 0)
	{
		return false;
	}

	// modify: �������ʽ������ж�
	auto wordsFounded = _words.find(key);
	if (wordsFounded != _words.end())
	{
		int64_t countPrevious = wordsFounded->second;
		count = (std::numeric_limits<int64_t>::max() - countPrevious > count)
			        ? (countPrevious + count)
			        : std::numeric_limits<int64_t>::max();
		wordsFounded->second = count;
		return false;
	}
	else
	{
		_words.insert({std::string(key), count});
	}

	//edits/suggestions are created only once, no matter how often word occurs
	//edits/suggestions are created only as soon as the word occurs in the corpus, 
	//even if the same term existed before in the dictionary as an edit from another word
	if (key.size() > this->_maxDictionaryWordLength)
	{
		this->_maxDictionaryWordLength = key.size();
	}
	//create deletes
	auto edits = EditsPrefix(key);
	// �����������д��
	for (auto it = edits.begin(); it != edits.end(); ++it)
	{
		int deleteHash = GetStringHash(*it);
		auto deletesFounded = _deletes.find(deleteHash);
		if (deletesFounded != _deletes.end())
		{
			auto& suggestions = deletesFounded->second;
			suggestions.push_back(std::string(key));
		}
		else
		{
			std::vector<std::string> suggestions = {std::string(key)};

			_deletes.insert({deleteHash, suggestions});
		}
	}
	return true;
}

std::vector<SuggestItem> SymSpell::LookUp(std::string_view input)
{
	std::vector<SuggestItem> suggestions;
	// early exit - word is too big to possibly match any words
	if (input.size() - 1 > _maxDictionaryWordLength)
	{
		return suggestions;
	}

	int64_t suggestionCount = 0;
	auto founded = _words.find(input);

	if (founded != _words.end())
	{
		suggestionCount = founded->second;
		suggestions.emplace_back(std::string(input), 0, suggestionCount);
	}

	// deletes we've considered already
	std::unordered_set<std::string> hashset1;
	// suggestions we've considered already
	std::unordered_set<std::string> hashset2;

	hashset2.insert(std::string(input));

	int maxEditDistance2 = 1;

	std::vector<std::string> candidates;

	auto inputPrefixLen = input.size();
	if (inputPrefixLen > _prefixLength)
	{
		inputPrefixLen = _prefixLength;
		candidates.emplace_back(input.substr(0, inputPrefixLen));
	}
	else
	{
		candidates.emplace_back(input);
	}

	std::size_t candidateIndex = 0;
	while (candidateIndex < candidates.size())
	{
		auto& candidate = candidates[candidateIndex++];
		auto candidateLen = candidate.size();
		int lengthDiff = inputPrefixLen - candidateLen;
		if (lengthDiff > maxEditDistance2)
		{
			break;
		}

		auto it = _deletes.find(GetStringHash(candidate));
		if (it != _deletes.end())
		{
			auto& dictSuggestions = it->second;
			for (auto& suggestion : dictSuggestions)
			{
				auto suggestionLen = suggestion.size();
				if (suggestion == input)
				{
					continue;
				}

				if ((::abs(static_cast<int>(suggestionLen - input.size())) > maxEditDistance2)
					|| (suggestionLen < candidateLen)
					|| (suggestionLen == candidateLen && suggestion != candidate))
				{
					continue;
				}

				auto suggPrefixLen = std::min(suggestionLen, _prefixLength);
				if (suggPrefixLen > inputPrefixLen && (suggPrefixLen - candidateLen) > maxEditDistance2) continue;

				//True Damerau-Levenshtein Edit Distance: adjust distance, if both distances>0
				//We allow simultaneous edits (deletes) of maxEditDistance on on both the dictionary and the input term. 
				//For replaces and adjacent transposes the resulting edit distance stays <= maxEditDistance.
				//For inserts and deletes the resulting edit distance might exceed maxEditDistance.
				//To prevent suggestions of a higher edit distance, we need to calculate the resulting edit distance, if there are simultaneous edits on both sides.
				//Example: (bank==bnak and bank==bink, but bank!=kanb and bank!=xban and bank!=baxn for maxEditDistance=1)
				//Two deletes on each side of a pair makes them all equal, but the first two pairs have edit distance=1, the others edit distance=2.
				int distance = 0;
				int minLen = 0;
				auto inputLen = input.size();
				if (candidateLen == 0)
				{
					//suggestions which have no common chars with input (inputLen<=maxEditDistance && suggestionLen<=maxEditDistance)
					distance = std::max(input.size(), suggestionLen);
					auto flag = hashset2.insert(suggestion);
					if (distance > maxEditDistance2 || !flag.second) continue;
				}
				else if (suggestionLen == 1)
				{
					// not entirely sure what happens here yet
					if (input.find(suggestion[0]) == input.npos)
						distance = input.size();
					else
						distance = input.size() - 1;

					auto flag = hashset2.insert(suggestion);
					if (distance > maxEditDistance2 || !flag.second) continue;
				}
				else

					//number of edits in prefix ==maxediddistance  AND no identic suffix
					//, then editdistance>maxEditDistance and no need for Levenshtein calculation  
					//      (inputLen >= prefixLength) && (suggestionLen >= prefixLength) 
					if ((_prefixLength - 1 == candidateLen)
						&& (((minLen = std::min(inputLen, suggestionLen) - _prefixLength) > 1)
							&& (input.substr(inputLen + 1 - minLen) != suggestion.substr(suggestionLen + 1 - minLen)))
						|| ((minLen > 0) && (input[inputLen - minLen] != suggestion[suggestionLen - minLen])
							&& ((input[inputLen - minLen - 1] != suggestion[suggestionLen - minLen])
								|| (input[inputLen - minLen] != suggestion[suggestionLen - minLen - 1]))))
					{
						continue;
					}
					else
					{
						// DeleteInSuggestionPrefix is somewhat expensive, and only pays off when verbosity is Top or Closest.
						if ((!DeleteInSuggestionPrefix(candidate, candidateLen, suggestion, suggestionLen))
							|| !hashset2.insert(suggestion).second)
							continue;
						distance = _editDistance.Compare(input, suggestion, maxEditDistance2);
						if (distance < 0) continue;
					}

				//save some time
				//do not process higher distances than those already found, if verbosity<All (note: maxEditDistance2 will always equal maxEditDistance when Verbosity.All)
				if (distance <= maxEditDistance2)
				{
					suggestionCount = _words[suggestion];
					SuggestItem si = SuggestItem(suggestion, distance, suggestionCount);
					if (suggestions.size() > 0)
					{
						if (distance < maxEditDistance2) suggestions.clear();
					}

					maxEditDistance2 = distance;
					suggestions.push_back(si);
				}
			}
		}
	}
	return suggestions;
}

std::unordered_set<std::string> SymSpell::EditsPrefix(std::string_view key)
{
	std::unordered_set<std::string> hashSet;
	if (key.size() <= _maxDictionaryWordLength)
	{
		hashSet.insert("");
	}
	if (key.size() > _prefixLength)
	{
		key = key.substr(0, _prefixLength);
	}
	hashSet.insert(std::string(key));
	Edits(key, hashSet);

	return hashSet;
}

void SymSpell::Edits(std::string_view word, std::unordered_set<std::string>& deleteWord)
{
	if (word.size() > 1)
	{
		for (std::size_t i = 0; i < word.size(); i++)
		{
			std::string tmp(word);
			auto del = tmp.erase(i, 1);
			// ����Ϊ1�ı༭����
			deleteWord.insert(del);
		}
	}
}

int SymSpell::GetStringHash(std::string_view source)
{
	uint32_t lenMask = static_cast<uint32_t>(source.size());
	if (lenMask > 3)
	{
		lenMask = 3;
	}
	uint32_t hash = 2166136261;
	for (auto c : source)
	{
		//unchecked, its fine even if it can be overflowed
		hash ^= c;
		hash *= 16777619;
	}

	hash &= _compactMask;
	hash |= lenMask;
	return static_cast<int>(hash);
}

bool SymSpell::DeleteInSuggestionPrefix(std::string_view deleteSugg, std::size_t deleteLen, std::string_view suggestion,
                                        std::size_t suggestionLen)
{
	if (deleteLen == 0)
	{
		return true;
	}
	if (_prefixLength < suggestionLen)
	{
		suggestionLen = _prefixLength;
	}

	std::size_t j = 0;
	for (std::size_t i = 0; i < deleteLen; i++)
	{
		char delChar = deleteSugg[i];
		while (j < suggestionLen && delChar != suggestion[j]) { j++; }
		if (j == suggestionLen)
		{
			return false;
		}
	}
	return true;
}

#ifdef USE_BUILDIN_DICTIONARY
#include "resources/dictionary.hpp"
#endif
