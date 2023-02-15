#include "Exchange.hpp"

#include "Misc/Bank.h"

// ----------------------------------------------------------------------------------------------------------------
// Base API

void YoshimiExchange::Bank::send_data(SynthEngine* synth, int action, int control, float value, int type, int part, int kititem, int engine, int insert, int parameter, int miscmsg)
{
    type |= TOPLEVEL::type::Write;
    collect_data(synth, value, action, type, control, part, kititem, engine, insert, parameter, UNUSED, miscmsg);
}

float YoshimiExchange::Bank::fetchData(SynthEngine* synth, float value, int control, int part, int kititem, int engine, int insert, int parameter, int offset, int miscmsg, int request)
{
    return collect_readData(synth, value, control, part, kititem, engine, insert, parameter, offset, miscmsg, request);
}

// ----------------------------------------------------------------------------------------------------------------
// Fetch bank information

void YoshimiExchange::Bank::getBankEntries(SynthEngine* synth, BankEntryMap& entryMap)
{
    ::Bank* bank = synth->getBankPtr();
    entryMap     = bank->getBanks(fetchData(synth, 0, BANK::control::selectRoot, TOPLEVEL::section::bank));
}

// For debug only
void YoshimiExchange::Bank::getBankNames(SynthEngine* synth, std::vector<std::string>& bankList)
{
    ::Bank* bank = synth->getBankPtr();
    bankList.clear();

    const BankEntryMap&          banks = bank->getBanks(fetchData(synth, 0, BANK::control::selectRoot, TOPLEVEL::section::bank));
    BankEntryMap::const_iterator it;
    for (it = banks.begin(); it != banks.end(); ++it) {
        if (!it->second.dirname.empty()) {
            bankList.push_back(it->second.dirname);
        }
    }
}

// For debug only
void YoshimiExchange::Bank::getBankIndexes(SynthEngine* synth, std::vector<long>& indexList)
{
    ::Bank* bank = synth->getBankPtr();
    indexList.clear();

    const BankEntryMap&          banks = bank->getBanks(fetchData(synth, 0, BANK::control::selectRoot, TOPLEVEL::section::bank));
    BankEntryMap::const_iterator it;
    for (it = banks.begin(); it != banks.end(); ++it) {
        if (!it->second.dirname.empty()) {
            indexList.push_back(it->first);
        }
    }
}

int YoshimiExchange::Bank::getCurrentBank(SynthEngine* synth)
{
    return fetchData(synth, 0, BANK::control::selectBank, TOPLEVEL::section::bank);
}

int YoshimiExchange::Bank::getCurrentInstrument(SynthEngine* synth)
{
    return fetchData(synth, 0, BANK::control::selectFirstInstrumentToSwap, TOPLEVEL::section::bank);
}

// ----------------------------------------------------------------------------------------------------------------
// Set bank state

void YoshimiExchange::Bank::switchBank(SynthEngine* synth, long newBankId)
{
    /*
     * NOTICE:
     * Both collect_data() and sendNormal() are OK.
     * But collect_data() implies TOPLEVEL::type::Write, which must be explicitly assigned in sendNormal().
     * Without TOPLEVEL::type::Write, sendNormal() will simply read data.
     */
    YoshimiExchange::collect_data(synth, newBankId, TOPLEVEL::action::lowPrio | TOPLEVEL::action::forceUpdate, TOPLEVEL::type::Integer, BANK::control::selectBank, TOPLEVEL::section::bank);
    // YoshimiExchange::sendNormal(synth, TOPLEVEL::action::lowPrio | TOPLEVEL::action::forceUpdate, newBankId, TOPLEVEL::type::Integer | TOPLEVEL::type::Write, BANK::control::selectBank, TOPLEVEL::section::bank);
}

void YoshimiExchange::Bank::switchInstrument(SynthEngine* synth, long newInstrumentId, int activePart)
{
    send_data(synth, TOPLEVEL::action::forceUpdate, MAIN::control::loadInstrumentFromBank, newInstrumentId, TOPLEVEL::type::Integer, TOPLEVEL::section::main, activePart);

    // The following two calls has the same effect as send_data()
    // YoshimiExchange::sendNormal(synth, TOPLEVEL::action::forceUpdate, newInstrumentId, TOPLEVEL::type::Integer | TOPLEVEL::type::Write, MAIN::control::loadInstrumentFromBank, TOPLEVEL::section::main, activePart);
    // YoshimiExchange::collect_data(synth, newInstrumentId, TOPLEVEL::action::forceUpdate, TOPLEVEL::type::Integer, MAIN::control::loadInstrumentFromBank, TOPLEVEL::section::main, activePart);
}
