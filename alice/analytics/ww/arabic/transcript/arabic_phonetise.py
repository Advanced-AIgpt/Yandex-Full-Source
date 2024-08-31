# код почти as is https://github.com/nawarhalabi/Arabic-Phonetiser
# изначально писалось на JS

import re
from arabic_buckwalter import arabic_to_buckwalter


def findStressIndex(sequence): #Find stress syllable in word starting from "start"
    if (sequence == u'' or len(sequence) == 0):
        return -1
    print(sequence)
    consonants = [
    "r", "g", "y","G",
    "b", "z", "f","v",
    "t", "s", "q","p",
    "$", "k", "<",
    "j", "S", "l",
    "H", "D", "m",
    "x", "T", "n",
    "d", "Z", "h",
    "*", "E", "w", "^"]
    geminatedConsonants = [
    "<<", "rr", "gg","vv",
    "bb", "zz", "ff","GG",
    "tt", "ss", "qq","pp",
    "$$", "kk", "yy",
    "jj", "SS", "ll",
    "HH", "DD", "mm",
    "xx", "TT", "nn",
    "dd", "ZZ", "hh",
    "**", "EE", "ww", "^^"]
    longVowels = ["aa", "AA",
    "uu0","uu1",
    "ii0","ii1",
    "UU0", "UU1",
    "II0", "II1"]
    shortVowels = ["a", "A",
    "u0","u1",
    "i0","i1",
    "U0", "U1",
    "I0", "I1"]
    syllableString = ""
    i = 0
    while i < len(sequence):
        if(sequence[i] in geminatedConsonants):
            syllableString += "C"
        elif(sequence[i] in consonants):
            syllableString += "c"
        elif(sequence[i] in longVowels):
            syllableString += "V"
        elif(sequence[i] in shortVowels):
            syllableString += "v"
        else:
            print('Unacceptable char when finding stress syllable: ' + sequence[i] +  ' ' + syllableString + '\n')
            file = open("errors", "a")
            file.write(sequence[i])
            file.close()
            return 0
        i += 1
    if(syllableString[0] in ['v', 'V']):
        return -1
    #Stress falls on the last syllable if it is super heavy
    if(syllableString.endswith("cVc") and syllableString.endswith("CVc")):
        return i - 2#3
    if(syllableString.endswith("cvvc") or syllableString.endswith("cvcc") or syllableString.endswith("cVcc") or syllableString.endswith("Cvvc") or syllableString.endswith("Cvcc") or syllableString.endswith("CVcc")):
        return i - 3#4
    if(syllableString.endswith("cvvcc") and syllableString.endswith("Cvvcc")):
        return i - 4#5
    #Stress is at the beginning if it is a monosyllabic word
    if(syllableString == "cvv" or syllableString == "cvc"):
        return i - 2#3
    if(syllableString == "cV"):
        return i - 1#2
    #Remove last syllable if first two rules miss
    if(syllableString.endswith("cvv") or syllableString.endswith("cvc")):
        syllableString = syllableString[0:-3]
        i = i - 3
    elif(syllableString.endswith("Cvv") or syllableString.endswith("Cvc")):
        syllableString = syllableString[0:-3]
        syllableString += 'c'
        i = i - 2
    elif(syllableString.endswith("cV") or syllableString.endswith("cv")):
        syllableString = syllableString[0:-2]
        i = i - 2
    elif(syllableString.endswith("CV") or syllableString.endswith("Cv")):
        syllableString = syllableString[0:-2]
        syllableString += 'c'
        i = i - 1
    #Stress is at penultimate syllable if disyllabic word
    if(syllableString == "cvv" or syllableString == "cvc"):
        return i - 2#3
    if(syllableString == "cV" or syllableString == "cv"):
        return i - 1#2
    #Stress is at penultimate syllable if it is heavy
    if(syllableString.endswith("cvv") or syllableString.endswith("cvc") or syllableString.endswith("Cvv") or syllableString.endswith("Cvc") or syllableString.endswith("cVc") or syllableString.endswith("cVC") or syllableString.endswith("CVc")):
        return i - 2#3
    if(syllableString.endswith("cV") or syllableString.endswith("CV")):
        return i - 1#2
    if(syllableString.endswith("cv")):
        i = i - 2
        syllableString = syllableString[0:-2]
    elif(syllableString.endswith("Cv")):
        i = i - 1
        syllableString = syllableString[0:-2]
        syllableString += 'c'
    #Stress is at antepenultimate syllable otherwise
    if(syllableString.endswith("cvv") or syllableString.endswith("cvc") or syllableString.endswith("Cvv") or syllableString.endswith("Cvc") or syllableString.endswith("cVc") or syllableString.endswith("cVC") or syllableString.endswith("CVc")):
        return i - 2#3
    if(syllableString.endswith("cV") or syllableString.endswith("cv") or syllableString.endswith("CV") or syllableString.endswith("Cv")):
        return i - 1#2
    return i + 1

# ----------------------------------------------------------------------------
# Grapheme to Phoneme mappings------------------------------------------------
# ----------------------------------------------------------------------------
unambiguousConsonantMap = {
    u'b': u'b', u'*': u'*', u'T': u'T', u'm': u'm',
    u't': u't', u'r': u'r', u'Z': u'Z', u'n': u'n',
    u'^': u'^', u'z': u'z', u'E': u'E', u'h': u'h',
    u'j': u'j', u's': u's', u'g': u'g', u'H': u'H',
    u'q': u'q', u'f': u'f', u'x': u'x', u'S': u'S',
    u'$': u'$', u'd': u'd', u'D': u'D', u'k': u'k',
    u'>': u'<', u'\'': u'<', u'}': u'<', u'&': u'<',
    u'<': u'<'
}

ambiguousConsonantMap = {
    u'l': [u'l', u''], u'w': u'w', u'y': u'y', u'p': [u't', u'']
    # These consonants are only unambiguous in certain contexts
}

maddaMap = {
    u'|': [[u'<', u'aa'], [u'<', u'AA']]
}

vowelMap = {
    u'A': [[u'aa', u''], [u'AA', u'']], u'Y': [[u'aa', u''], [u'AA', u'']],
    u'w': [[u'uu0', u'uu1'], [u'UU0', u'UU1']],
    u'y': [[u'ii0', u'ii1'], [u'II0', u'II1']],
    u'a': [u'a', u'A'],
    u'u': [[u'u0', u'u1'], [u'U0', u'U1']],
    u'i': [[u'i0', u'i1'], [u'I0', u'I1']],
}

nunationMap = {
    u'F': [[u'a', u'n'], [u'A', u'n']], u'N': [[u'u1', u'n'], [u'U1', u'n']], u'K': [[u'i1', u'n'], [u'I1', u'n']]
}

diacritics = [u'o', u'a', u'u', u'i', u'F', u'N', u'K', u'~']
diacriticsWithoutShadda = [u'o', u'a', u'u', u'i', u'F', u'N', u'K']
emphatics = [u'D', u'S', u'T', u'Z', u'g', u'x', u'q']
forwardEmphatics = [u'g', u'x']
consonants = [u'>', u'<', u'}', u'&', u'\'', u'b', u't', u'^', u'j', u'H', u'x', u'd', u'*', u'r', u'z', u's', u'$',
              u'S', u'D', u'T', u'Z', u'E', u'g', u'f', u'q', u'k', u'l', u'm', u'n', u'h', u'|']

# ------------------------------------------------------------------------------------
# Words with fixed irregular pronunciations-------------------------------------------
# ------------------------------------------------------------------------------------
fixedWords = {
    u'h*A': [u'h aa * aa', u'h aa * a', ],
    u'bh*A': [u'b i0 h aa * aa', u'b i0 h aa * a', ],
    u'kh*A': [u'k a h aa * aa', u'k a h aa * a', ],
    u'fh*A': [u'f a h aa * aa', u'f a h aa * a', ],
    u'h*h': [u'h aa * i0 h i0', u'h aa * i1 h'],
    u'bh*h': [u'b i0 h aa * i0 h i0', u'b i0 h aa * i1 h'],
    u'kh*h': [u'k a h aa * i0 h i0', u'k a h aa * i1 h'],
    u'fh*h': [u'f a h aa * i0 h i0', u'f a h aa * i1 h'],
    u'h*An': [u'h aa * aa n i0', u'h aa * aa n'],
    u'h&lA\'': [u'h aa < u0 l aa < i0', u'h aa < u0 l aa <'],
    u'*lk': [u'* aa l i0 k a', u'* aa l i0 k'],
    u'b*lk': [u'b i0 * aa l i0 k a', u'b i0 * aa l i0 k'],
    u'k*lk': [u'k a * aa l i0 k a', u'k a * aa l i1 k'],
    u'*lkm': u'* aa l i0 k u1 m',
    u'>wl}k': [u'< u0 l aa < i0 k a', u'< u0 l aa < i1 k'],
    u'Th': u'T aa h a',
    u'lkn': [u'l aa k i0 nn a', u'l aa k i1 n'],
    u'lknh': u'l aa k i0 nn a h u0',
    u'lknhm': u'l aa k i0 nn a h u1 m',
    u'lknk': [u'l aa k i0 nn a k a', u'l aa k i0 nn a k i0'],
    u'lknkm': u'l aa k i0 nn a k u1 m',
    u'lknkmA': u'l aa k i0 nn a k u0 m aa',
    u'lknnA': u'l aa k i0 nn a n aa',
    u'AlrHmn': [u'rr a H m aa n i0', u'rr a H m aa n'],
    u'Allh': [u'll aa h i0', u'll aa h', u'll AA h u0', u'll AA h a', u'll AA h', u'll A'],
    u'h*yn': [u'h aa * a y n i0', u'h aa * a y n'],

    u'wh*A': [u'w a h aa * aa', u'w a h aa * a', ],
    u'wbh*A': [u'w a b i0 h aa * aa', u'w a b i0 h aa * a', ],
    u'wkh*A': [u'w a k a h aa * aa', u'w a k a h aa * a', ],
    u'wh*h': [u'w a h aa * i0 h i0', u'w a h aa * i1 h'],
    u'wbh*h': [u'w a b i0 h aa * i0 h i0', u'w a b i0 h aa * i1 h'],
    u'wkh*h': [u'w a k a h aa * i0 h i0', u'w a k a h aa * i1 h'],
    u'wh*An': [u'w a h aa * aa n i0', u'w a h aa * aa n'],
    u'wh&lA\'': [u'w a h aa < u0 l aa < i0', u'w a h aa < u0 l aa <'],
    u'w*lk': [u'w a * aa l i0 k a', u'w a * aa l i0 k'],
    u'wb*lk': [u'w a b i0 * aa l i0 k a', u'w a b i0 * aa l i0 k'],
    u'wk*lk': [u'w a k a * aa l i0 k a', u'w a k a * aa l i1 k'],
    u'w*lkm': u'w a * aa l i0 k u1 m',
    u'w>wl}k': [u'w a < u0 l aa < i0 k a', u'w a < u0 l aa < i1 k'],
    u'wTh': u'w a T aa h a',
    u'wlkn': [u'w a l aa k i0 nn a', u'w a l aa k i1 n'],
    u'wlknh': u'w a l aa k i0 nn a h u0',
    u'wlknhm': u'w a l aa k i0 nn a h u1 m',
    u'wlknk': [u'w a l aa k i0 nn a k a', u'w a l aa k i0 nn a k i0'],
    u'wlknkm': u'w a l aa k i0 nn a k u1 m',
    u'wlknkmA': u'w a l aa k i0 nn a k u0 m aa',
    u'wlknnA': u'w a l aa k i0 nn a n aa',
    u'wAlrHmn': [u'w a rr a H m aa n i0', u'w a rr a H m aa n'],
    u'wAllh': [u'w a ll aa h i0', u'w a ll aa h', u'w a ll AA h u0', u'w a ll AA h a', u'w a ll AA h', u'w a ll A'],
    u'wh*yn': [u'w a h aa * a y n i0', u'w a h aa * a y n'],
    u'w': [u'w a'],
    u'Aw': [u'< a w'],
    u'>w': [u'< a w'],

    u'Alf': [u'< a l f'],
    u'>lf': [u'< a l f'],
    u'b>lf': [u'b i0 < a l f'],
    u'f>lf': [u'f a < a l f'],
    u'wAlf': [u'w a < a l f'],
    u'w>lf': [u'w a < a l f'],
    u'wb>lf': [u'w a b i0 < a l f'],

    u'nt': u'n i1 t',
    u'fydyw': u'v i0 d y uu1',
    u'lndn': u'l A n d u1 n'
}


def isFixedWord(word, results, orthography, pronunciations):
    lastLetter = ''
    if (len(word) > 0):
        lastLetter = word[-1]
    if (lastLetter == u'a'):
        lastLetter = [u'a', u'A']
    elif (lastLetter == u'A'):
        lastLetter = [u'aa']
    elif (lastLetter == u'u'):
        lastLetter = [u'u0']
    elif (lastLetter == u'i'):
        lastLetter = [u'i0']
    elif (lastLetter in unambiguousConsonantMap):
        lastLetter = [unambiguousConsonantMap[lastLetter]]
    wordConsonants = re.sub(u'[^h*Ahn\'>wl}kmyTtfdb]', u'', word)  # Remove all dacritics from word
    if (wordConsonants in fixedWords):  # check if word is in the fixed word lookup table
        if (isinstance(fixedWords[wordConsonants], list)):
            done = False
            for pronunciation in fixedWords[wordConsonants]:
                if (pronunciation.split(u' ')[-1] in lastLetter):
                    results += word + u' ' + pronunciation + u'\n'  # add each pronunciation to the pronunciation dictionary
                    pronunciations.append(pronunciation.split(u' '))
                    done = True
            if (not done):
                results += word + u' ' + fixedWords[wordConsonants][
                    0] + u'\n'  # add each pronunciation to the pronunciation dictionary
                pronunciations.append(fixedWords[wordConsonants][0].split(u' '))
        else:
            results += word + u' ' + fixedWords[
                wordConsonants] + u'\n'  # add pronunciation to the pronunciation dictionary
            pronunciations.append(fixedWords[wordConsonants].split(u' '))
    return results


def phonetise(utterances):
    result = u''  # Pronunciations Dictionary
    utterancesPronuncations = []  # Most likely pronunciation for all utterances
    utterancesPronuncationsWithBoundaries = []  # Most likely pronunciation for all utterances

    # -----------------------------------------------------------------------------------------------------
    # Loop through utterances------------------------------------------------------------------------------
    # -----------------------------------------------------------------------------------------------------
    utteranceNumber = 1
    for utterance in utterances:
        utteranceNumber += 1
        utterancesPronuncations.append('')  # Add empty entry that will hold this utterance's pronuncation
        utterancesPronuncationsWithBoundaries.append('')  # Add empty entry that will hold this utterance's pronuncation

        utterance = arabic_to_buckwalter(utterance)
        print(u"phoetising utterance")
        print(utterance)
        # Do some normalisation work and split utterance to words
        utterance = utterance.replace(u'AF', u'F')
        utterance = utterance.replace(u'\u0640', u'')
        utterance = utterance.replace(u'o', u'')
        utterance = utterance.replace(u'aA', u'A')
        utterance = utterance.replace(u'aY', u'Y')
        utterance = re.sub(u'([^\\-]) A', u'\\1 ', utterance)
        utterance = utterance.replace(u'F', u'an')
        utterance = utterance.replace(u'N', u'un')
        utterance = utterance.replace(u'K', u'in')
        utterance = utterance.replace(u'|', u'>A')

        # Deal with Hamza types that when not followed by a short vowel letter,
        # this short vowel is added automatically
        utterance = re.sub(u'^Ai', u'<i', utterance)
        utterance = re.sub(u'^Aa', u'>a', utterance)
        utterance = re.sub(u'^Au', u'>u', utterance)
        utterance = re.sub(u'Ai', u'<i', utterance)
        utterance = re.sub(u'Aa', u'>a', utterance)
        utterance = re.sub(u'Au', u'>u', utterance)
        utterance = re.sub(u'^Al', u'>al', utterance)
        utterance = re.sub(u' - Al', u' - >al', utterance)
        utterance = re.sub(u'^- Al', u'- >al', utterance)
        utterance = re.sub(u'^>([^auAw])', u'>a\\1', utterance)
        utterance = re.sub(u' >([^auAw ])', u' >a\\1', utterance)
        utterance = re.sub(u'<([^i])', u'<i\\1', utterance)
        utterance = re.sub(u' A([^aui])', u' \\1', utterance)
        utterance = re.sub(u'^A([^aui])', u'\\1', utterance)

        utterance = utterance.split(u' ')
        # ---------------------------
        wordIndex = -1

        # Loop through words
        for word in utterance:
            wordIndex += 1
            if (not word in [u'-', u'sil']):
                pronunciations = []  # Start with empty set of possible pronunciations of current word
                result = isFixedWord(word, result, word,
                                     pronunciations)  # Add fixed irregular pronunciations if possible

                emphaticContext = False  # Indicates whether current character is in an emphatic context or not. Starts with False
                word = u'bb' + word + u'ee'  # This is the end/beginning of word symbol. just for convenience

                phones = []  # Empty list which will hold individual possible word's pronunciation

                # -----------------------------------------------------------------------------------
                # MAIN LOOP: here is where the Modern Standard Arabic phonetisation rule-set starts--
                # -----------------------------------------------------------------------------------
                for index in range(2, len(word) - 2):
                    letter = word[index]  # Current Character
                    letter1 = word[index + 1]  # Next Character
                    letter2 = word[index + 2]  # Next-Next Character
                    letter_1 = word[index - 1]  # Previous Character
                    letter_2 = word[index - 2]  # Before Previous Character
                    # ----------------------------------------------------------------------------------------------------------------
                    if (letter in consonants + [u'w', u'y'] and not letter in emphatics + [
                        u'r'""", u'l'"""]):  # non-emphatic consonants (except for Lam and Ra) change emphasis back to False
                        emphaticContext = False
                    if (letter in emphatics):  # Emphatic consonants change emphasis context to True
                        emphaticContext = True
                    if (
                            letter1 in emphatics and not letter1 in forwardEmphatics):  # If following letter is backward emphatic, emphasis state is set to True
                        emphaticContext = True
                    # ----------------------------------------------------------------------------------------------------------------
                    # ----------------------------------------------------------------------------------------------------------------
                    if (
                            letter in unambiguousConsonantMap):  # Unambiguous consonant phones. These map to a predetermined phoneme
                        phones += [unambiguousConsonantMap[letter]]
                    # ----------------------------------------------------------------------------------------------------------------
                    if (letter == u'l'):  # Lam is a consonant which requires special treatment
                        if ((not letter1 in diacritics and not letter1 in vowelMap) and letter2 in [u'~'] and (
                                (letter_1 in [u'A', u'l', u'b']) or (letter_1 in diacritics and letter_2 in [u'A', u'l',
                                                                                                             u'b']))):  # Lam could be omitted in definite article (sun letters)
                            phones += [ambiguousConsonantMap[u'l'][1]]  # omit
                        else:
                            phones += [ambiguousConsonantMap[u'l'][0]]  # do not omit
                    # ----------------------------------------------------------------------------------------------------------------
                    if (letter == u'~' and not letter_1 in [u'w', u'y'] and len(
                            phones) > 0):  # shadda just doubles the letter before it
                        phones[-1] += phones[-1]
                    # ----------------------------------------------------------------------------------------------------------------
                    if (letter == u'|'):  # Madda only changes based in emphaticness
                        if (emphaticContext):
                            phones += [maddaMap[u'|'][1]]
                        else:
                            phones += [maddaMap[u'|'][0]]
                    # ----------------------------------------------------------------------------------------------------------------
                    if (letter == u'p'):  # Ta' marboota is determined by the following if it is a diacritic or not
                        if (letter1 in diacritics):
                            phones += [ambiguousConsonantMap[u'p'][0]]
                        else:
                            phones += [ambiguousConsonantMap[u'p'][1]]
                    # ----------------------------------------------------------------------------------------------------------------
                    if (letter in vowelMap):
                        if (letter in [u'w',
                                       u'y']):  # Waw and Ya are complex they could be consonants or vowels and their gemination is complex as it could be a combination of a vowel and consonants
                            if (letter1 in diacriticsWithoutShadda + [u'A', u'Y'] or (
                                    letter1 in [u'w', u'y'] and not letter2 in diacritics + [u'A', u'w', u'y']) or (
                                    letter_1 in diacriticsWithoutShadda and letter1 in consonants + [u'e'])):
                                if ((letter in [u'w'] and letter_1 in [u'u'] and not letter1 in [u'a', u'i', u'A',
                                                                                                 u'Y']) or (
                                        letter in [u'y'] and letter_1 in [u'i'] and not letter1 in [u'a', u'u', u'A',
                                                                                                    u'Y'])):
                                    if (emphaticContext):
                                        phones += [vowelMap[letter][1][0]]
                                    else:
                                        phones += [vowelMap[letter][0][0]]
                                else:
                                    if (letter1 in [u'A'] and letter in [u'w'] and letter2 in [u'e']):
                                        phones += [[vowelMap[letter][0][0], ambiguousConsonantMap[letter]]]
                                    else:
                                        phones += [ambiguousConsonantMap[letter]]
                            elif (letter1 in [u'~']):
                                if (letter_1 in [u'a'] or (letter in [u'w'] and letter_1 in [u'i', u'y']) or (
                                        letter in [u'y'] and letter_1 in [u'w', u'u'])):
                                    phones += [ambiguousConsonantMap[letter], ambiguousConsonantMap[letter]]
                                else:
                                    phones += [vowelMap[letter][0][0], ambiguousConsonantMap[letter]]
                            else:  # Waws and Ya's at the end of the word could be shortened
                                if (emphaticContext):
                                    if (letter_1 in consonants + [u'u', u'i'] and letter1 in [u'e']):
                                        phones += [[vowelMap[letter][1][0], vowelMap[letter][1][0][1:]]]
                                    else:
                                        phones += [vowelMap[letter][1][0]]
                                else:
                                    if (letter_1 in consonants + [u'u', u'i'] and letter1 in [u'e']):
                                        phones += [[vowelMap[letter][0][0], vowelMap[letter][0][0][1:]]]
                                    else:
                                        phones += [vowelMap[letter][0][0]]
                        if (letter in [u'u',
                                       u'i']):  # Kasra and Damma could be mildened if before a final silent consonant
                            if (emphaticContext):
                                if ((letter1 in unambiguousConsonantMap or letter1 == u'l') and letter2 == u'e' and len(
                                        word) > 7):
                                    phones += [vowelMap[letter][1][1]]
                                else:
                                    phones += [vowelMap[letter][1][0]]
                            else:
                                if ((letter1 in unambiguousConsonantMap or letter1 == u'l') and letter2 == u'e' and len(
                                        word) > 7):
                                    phones += [vowelMap[letter][0][1]]
                                else:
                                    phones += [vowelMap[letter][0][0]]
                        if (letter in [u'a', u'A',
                                       u'Y']):  # Alif could be ommited in definite article and beginning of some words
                            if (letter in [u'A'] and letter_1 in [u'w', u'k'] and letter_2 == u'b' and letter1 in [
                                u'l']):
                                phones += [[u'a', vowelMap[letter][0][0]]]
                            elif (letter in [u'A'] and letter_1 in [u'u', u'i']):
                                temp = True  # do nothing
                            elif (letter in [u'A'] and letter_1 in [u'w'] and letter1 in [
                                u'e']):  # Waw al jama3a: The Alif after is optional
                                phones += [[vowelMap[letter][0][1], vowelMap[letter][0][0]]]
                            elif (letter in [u'A', u'Y'] and letter1 in [u'e']):
                                if (emphaticContext):
                                    phones += [[vowelMap[letter][1][0], vowelMap[u'a'][1]]]
                                else:
                                    phones += [[vowelMap[letter][0][0], vowelMap[u'a'][0]]]
                            else:
                                if (emphaticContext):
                                    phones += [vowelMap[letter][1][0]]
                                else:
                                    phones += [vowelMap[letter][0][0]]
                # -------------------------------------------------------------------------------------------------------------------------
                # End of main loop---------------------------------------------------------------------------------------------------------
                # -------------------------------------------------------------------------------------------------------------------------
                possibilities = 1  # Holds the number of possible pronunciations of a word

                # count the number of possible pronunciations
                for letter in phones:
                    if (isinstance(letter, list)):
                        possibilities = possibilities * len(letter)

                # Generate all possible pronunciations
                for i in range(0, possibilities):
                    pronunciations.append([])
                    iterations = 1
                    for index, letter in enumerate(phones):
                        if (isinstance(letter, list)):
                            curIndex = int((i / iterations) % len(letter))
                            if (letter[curIndex] != u''):
                                pronunciations[-1].append(letter[curIndex])
                            iterations = iterations * len(letter)
                        else:
                            if (letter != u''):
                                pronunciations[-1].append(letter)

                # Iterate through each pronunciation to perform some house keeping. And append pronunciation to dictionary
                # 1- Remove duplicate vowels
                # 2- Remove duplicate y and w
                for pronunciation in pronunciations:
                    prevLetter = u''
                    toDelete = []
                    for i in range(0, len(pronunciation)):
                        letter = pronunciation[i]
                        if (letter in [u'aa', u'uu0', u'ii0', u'AA', u'UU0', u'II0'] and prevLetter.lower() == letter[
                                                                                                               1:].lower()):  # Delete duplicate consecutive vowels
                            toDelete.append(i - 1)
                            pronunciation[i] = pronunciation[i - 1][0] + pronunciation[i - 1]
                        if (letter in [u'u0', u'i0'] and prevLetter.lower() == letter.lower()):  # Delete duplicates
                            toDelete.append(i - 1)
                            pronunciation[i] = pronunciation[i - 1]
                        if (letter in [u'y', u'w'] and prevLetter == letter):  # delete duplicate
                            pronunciation[i - 1] += pronunciation[i - 1]
                            toDelete.append(i)
                        if (letter in [u'a'] and prevLetter == letter):  # delete duplicate
                            toDelete.append(i)

                        prevLetter = letter
                    for i in reversed(range(0, len(toDelete))):
                        del (pronunciation[toDelete[i]])
                    result += word[2:-2] + u' ' + u' '.join(pronunciation) + u'\n'

                # Append utterance pronunciation to utterancesPronunciations
                utterancesPronuncations[-1] += u" " + u" ".join(pronunciations[0])

                # Add Stress to each pronunciation
                pIndex = 0
                for pronunciation in pronunciations:
                    stressIndex = findStressIndex(pronunciation)
                    if (stressIndex < len(pronunciation) and stressIndex != -1):
                        pronunciation[stressIndex] += u'\''
                    else:
                        if (pIndex == 0):
                            print('skipped')
                            print(pronunciation)

                    pIndex += 1
                # Append utterance pronunciation to utterancesPronunciations
                utterancesPronuncationsWithBoundaries[-1] += u" " + u"".join(pronunciations[0])
            else:
                utterancesPronuncations[-1] += u" sil"
                utterancesPronuncationsWithBoundaries[-1] += u" sil"

        # Add sound file name back
        utterancesPronuncations[-1] = utterancesPronuncations[-1].strip()  # + u" sil"
        utterancesPronuncationsWithBoundaries[-1] = utterancesPronuncationsWithBoundaries[-1].strip()  # + u" sil"

    return (utterancesPronuncationsWithBoundaries, utterancesPronuncations, result)
