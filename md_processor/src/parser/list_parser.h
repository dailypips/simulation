
#include <stdint.h>
#include <sys/time.h>
#include <algorithm>
#include <array>

#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <limits>
#include <string>
#include <tuple>

#include <utility>

#include "md_types.h"
#include "md_stats.h"
#include "md_helper.h"

#include "md_token_types.h"

/**
 * @brief List parser implements a model assuming that the tokens that is expects are in a
 * simple list structure, ie one after another and we know something about the order.
 * So an example is a csv format A,1000001,B,5,900 or ["A",1000001,"B",5,900]
 * The tokens can be identified from the separator ',' and extracted as a token list
 * with a certain order i.e event,orderid,side,quantity,price
 *
 * More complex parser models are implemented in betfair_parser etc
 *
 */
struct list_parser {

/**
 *  Token List based Event type parsing
 *
 * @param itr
 * @param event
 * @return
 */
template <typename Iterator>
static bool parse_type(Iterator& message_iter,const Iterator& end,Event & event) {
    return make_type(message_iter,end, event);
}

/**
 * Token List based for Side parsing
 *
 * @param itr
 * @param side
 * @return True if syntax OK
 */
template <typename Iterator>
static bool parse_type(Iterator& message_iter,const Iterator& end,Side & side) {
    return make_type(message_iter,end, side);
}

/**
 * Parse to extract a variable list and apply parsing result type
 *
 * @param itr
 * @param t
 * @return True if syntax OK
 */
template <typename T, typename Iterator, typename... Rest>
static bool parse_type(Iterator& message_iter,const Iterator& end,
                  T& t,
                  Rest&... rest)
{
	return parse_type(message_iter,end,t) && parse_type(++message_iter,end,rest...);
}

/**
 * Parse to extract a single templated result type
 *
 * @param itr
 * @param t
 * @return True if syntax OK
 */
template <typename T, typename Iterator>
static bool parse_type(Iterator& message_iter,const Iterator& end,
                  T& t)
{
	return make_type(message_iter,end,t);
}

/**
 * Extracts the event type from the token list message event
 * @param message_iter
 * @return True if syntax OK
 */
template <typename Iterator>
static auto get_event(Iterator &message_iter, const Iterator &end) -> Event {
	Event event{Event::Unknown};
	parse_type(message_iter,end,event);

    if (!contains(event,'A','M','X','T','S')) {
        stats().event_error();
        throw std::runtime_error("Corruption");
    }

    return event;
}

/**
 * Extracts the order object properties from the token list  message event
 *
 * @param message_iter
 * @return
 */
template <typename Iterator>
static auto get_order(Iterator &message_iter, const Iterator &end) -> Order {
	// Order id extract
	OrderIdKeyType orderid{};
    if(likely(parse_type(++message_iter,end,orderid))) {
	   if (!(orderid > 0 && orderid <= max_order_id)) {
		   stats().order_range();
		   throw std::runtime_error("Order id range");
	   }
    }
    else {
    	stats().order_parse();
    	throw std::runtime_error("Order id syntax");
    }

    // Side extract
    Side side{Side::Unknown};
	if (unlikely(!parse_type(++message_iter,end,side))) {
    	stats().side_error();
    	throw std::runtime_error("Order side syntax");
    }

    // Quantity extract
    QuantityValueType quantity{};
	if (likely(parse_type(++message_iter,end,quantity))) {
		if (!(quantity > 0 && quantity <= max_order_quantity)) {
			stats().quantity_range();
			throw std::runtime_error("Order quantity range");
		}
	}
	else {
		stats().quantity_parse();
		throw std::runtime_error("Order quantity syntax");
	}

    // Price extract
	PriceLevelKey price{};
	if (likely(parse_type(++message_iter,end,price))) {
		if (!(price > 0 && price <= max_order_price)) {
			stats().price_range();
			throw std::runtime_error("Order price range");
		}
	}
	else {
		stats().price_parse();
		throw std::runtime_error("Order price syntax");
	}

	// I am a pod so optimizer do job
    return Order{orderid,side,quantity,price};
}

/**
 * Extracts the trade properties from the text message event
 *
 * @param message_iter
 * @return
 */
template <typename Iterator>
static auto get_trade(Iterator &message_iter, const Iterator &end) -> Trade {
    // Side extract
    Side side{Side::Unknown};
	if (unlikely(!parse_type(++message_iter,end,side))) {
    	stats().side_error();
    	throw std::runtime_error("Order side syntax");
    }

    QuantityValueType quantity{};
	if (likely(parse_type(++message_iter,end,quantity))) {
		if (quantity<=0) {
			stats().quantity_range();
			throw std::runtime_error("Trade quantity range"+quantity);
		}
	}
	else {
		stats().quantity_parse();
		throw std::runtime_error("Trade quantity syntax");
	}

	PriceLevelKey price{};
	if (likely(parse_type(++message_iter,end,price))) {
		if (price<=0) {
			stats().price_parse();
			throw std::runtime_error("Trade price range"+price);
		}
	}
	else {
		stats().price_parse();
		throw std::runtime_error("Trade price is syntax");
	}

    return Trade{side,quantity,price};
}



/**
 * Extracts the bid order side object properties from the token list message event
 *
 * @param message_iter
 * @return
 */
template <typename Iterator>
static auto get_snapshot_bid_order(Iterator &message_iter, const Iterator &end) -> Order {
	// Order id extract
	OrderIdKeyType orderid{};

	// Number of contributors in the case of a some simulated snapshots may be set to always 1
	int num=0;
	if (likely(!parse_type(++message_iter,end,num))) {
		throw std::runtime_error("Order number of contributors syntax");
	}

    // Quantity extract
    QuantityValueType quantity{};
	if (likely(parse_type(++message_iter,end,quantity))) {
		if (!(quantity > 0 && quantity <= max_order_quantity)) {
			stats().quantity_range();
			throw std::runtime_error("Order quantity range");
		}
	}
	else {
		stats().quantity_parse();
		throw std::runtime_error("Order quantity syntax");
	}

    // Price extract
	PriceLevelKey price{};
	//double dprice;
	if (likely(parse_type(++message_iter,end,price))) {
		//price=static_cast<PriceLevelKey>(dprice);
		if (!(price > 0 && price <= max_order_price)) {
			stats().price_range();
			throw std::runtime_error("Order price range");
		}
	}
	else {
		stats().price_parse();
		throw std::runtime_error("Order price syntax");
	}

	// In the snapshot, the order is everything accumulated, so we have a
	// synthetic order id
	orderid=(1e6+price);
	// I am a pod so optimizer do job
    return Order{orderid,Side::Bid,quantity,price};
}

/**
 * Extracts the bid order side object properties from the text message event
 *
 * @param message_iter
 * @return
 */
template <typename Iterator>
static auto get_snapshot_ask_order(Iterator &message_iter, const Iterator &end) -> Order {
	// Order id extract
	OrderIdKeyType orderid{};

    // Price extract
	PriceLevelKey price{};

	if (likely(parse_type(++message_iter,end,price))) {
		if (!(price > 0 && price <= max_order_price)) {
			stats().price_range();
			throw std::runtime_error("Order price range");
		}
	}
	else {
		stats().price_parse();
		throw std::runtime_error("Order price syntax");
	}

    // Quantity extract
    QuantityValueType quantity{};
	if (likely(parse_type(++message_iter,end,quantity))) {
		if (!(quantity > 0 && quantity <= max_order_quantity)) {
			stats().quantity_range();
			throw std::runtime_error("Order quantity range");
		}
	}
	else {
		stats().quantity_parse();
		throw std::runtime_error("Order quantity syntax");
	}


	// Number of contributors in the case of a snapshot is set to always 1
	int num=0;
	if (likely(!parse_type(++message_iter,end,num))) {
		throw std::runtime_error("Order number of contributors syntax");
	}

	// In the snapshot, the order is everything accumulated, so we have a
	// synthetic order id
	orderid=(2e6+price);
	// I am a pod so optimizer do job
    return Order{orderid,Side::Ask,quantity,price};
}


/**
 * Get the orders that will make up a snapshot
 * This assumes a token list format of the order book
 *
 * @param message_iter
 * @param end
 * @return
 */
template <typename Iterator>
static auto get_orders(Iterator &message_iter, const Iterator &end) -> Orders {
	Orders orders;
	while((message_iter+1) != end) {
		orders.emplace_back(get_snapshot_bid_order(message_iter,end));
		orders.emplace_back(get_snapshot_ask_order(message_iter,end));
	}
	// Mr compiler lets have some RVO please
    return orders;
}

/**
 * Extracts the trade properties from the text message event
 *
 * @param message_iter
 * @return
 */
template <typename Iterator>
static auto get_snapshot_trade(Iterator &message_iter, const Iterator &end) -> Trade {
    // Side extract
    Side side{Side::Unknown};
    QuantityValueType quantity{};
	PriceLevelKey price{};

    if (parse_type(++message_iter,end,side)) {
		if (likely(parse_type(++message_iter,end,quantity))) {
			if (quantity<=0) {
				stats().quantity_range();
				throw std::runtime_error("Trade quantity range"+quantity);
			}
		}
		else {
			stats().quantity_parse();
			throw std::runtime_error("Trade quantity syntax");
		}

		if (likely(parse_type(++message_iter,end,price))) {
			if (price<=0) {
				stats().price_parse();
				throw std::runtime_error("Trade price range"+price);
			}
		}
		else {
			stats().price_parse();
			throw std::runtime_error("Trade price is syntax");
		}
    }
    else {
    	message_iter+=2;
    }

    return Trade{side,quantity,price};
}

/**
 * Get the trades that will make up a snapshot
 *
 * @param message_iter
 * @param end
 * @return
 */
template <typename Iterator>
static auto get_trades(Iterator &message_iter, const Iterator &end) -> Trades {
	Trades trades;
	Trade trade = get_snapshot_trade(message_iter,end);

	// Do we have an trade with snapshot
	if (unlikely(trade.side!=Side::Unknown)) {
		// Yes, so extract the trade
		trades.emplace_back(trade);
	}

	// Mr compiler lets have some RVO please
    return trades;
}

};

