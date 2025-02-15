#include "States/Over.hpp"

#include "Core/Game.hpp"
#include "Core/Platform.hpp"
#include "Core/Font.hpp"
#include "Factories/LevelFactory.hpp"
#include "Objects/Level.hpp"
#include "States/Load.hpp"
#include "States/Menu.hpp"
#include "States/Play.hpp"
#include "States/Quit.hpp"

#include <cstring>
#include <ostream>
#include <fstream>

namespace SuperHaxagon {

	Over::Over(Game& game, std::unique_ptr<Level> level, LevelFactory& selected, const float score, std::string text) :
		_game(game),
		_platform(game.getPlatform()),
		_selected(selected),
		_level(std::move(level)),
		_text(std::move(text)),
		_score(score) {
		_high = _selected.setHighScore(static_cast<int>(score));
	}

	Over::~Over() = default;

	void Over::enter() {
		_platform.playSFX(_game.getSFXOver());

		std::ofstream scores(_platform.getPath("/scores.db", Location::USER), std::ios::out | std::ios::binary);

		if (!scores) return;

		scores.write(Load::SCORE_HEADER, strlen(Load::SCORE_HEADER));
		auto levels = static_cast<uint32_t>(_game.getLevels().size());
		scores.write(reinterpret_cast<char*>(&levels), sizeof(levels));

		for (const auto& lev : _game.getLevels()) {
			writeString(scores, lev->getName());
			writeString(scores, lev->getDifficulty());
			writeString(scores, lev->getMode());
			writeString(scores, lev->getCreator());
			uint32_t highSc = lev->getHighScore();
			scores.write(reinterpret_cast<char*>(&highSc), sizeof(highSc));
		}

		scores.write(Load::SCORE_FOOTER, strlen(Load::SCORE_FOOTER));
	}

	std::unique_ptr<State> Over::update(const float dilation) {
		_frames += dilation;
		_level->rotate(GAME_OVER_ROT_SPEED, dilation);
		_level->clamp();

		const auto press = _platform.getPressed();
		if(press.quit) return std::make_unique<Quit>(_game);

		if(_frames <= FRAMES_PER_GAME_OVER) {
			_offset *= GAME_OVER_ACCELERATION_RATE * dilation + 1.0f;
		}

		if(_frames >= FRAMES_PER_GAME_OVER) {
			_level->clearPatterns();
			if (press.game_left || press.game_right) {
				// If the level we are playing is not the same as the index, we need to load
				// the original music
				if (_selected.getMusic() != _level->getLevelFactory().getMusic()) {
					_game.loadBGMAudio(_selected.getMusic(), _selected.getLocation(), true);
				}

				// Go back to the original level
				return std::make_unique<Play>(_game, _selected, _selected, 0.0f);
			}

			if (press.actual_select) {
				return std::make_unique<Menu>(_game, _selected);
			}
		}

		return nullptr;
	}

	void Over::drawTop(const float scale) {
		_level->draw(_game, scale, _offset);
	}

	void Over::drawBot(const float scale) {
		auto& large = _game.getFontLarge();
		auto& small = _game.getFontSmall();
		large.setScale(scale);
		small.setScale(scale);

		const auto padText = 3 * scale;
		const auto margin = 20 * scale;
		const auto width = _platform.getScreenDim().x;
		const auto height = _platform.getScreenDim().y;
		const auto heightLarge = large.getHeight();
		const auto heightSmall = small.getHeight();

		const Point posGameOver = {width / 2, margin};
		const Point posTime = {width / 2, posGameOver.y + heightLarge + padText};
		const Point posBest = {width / 2, posTime.y + heightSmall + padText};
		const Point posB = {width / 2, height - margin - heightSmall};
		const Point posA = {width / 2, posB.y - heightSmall - padText};

		const auto textScore = std::string("TIME: ") + getTime(_score);
		large.draw(COLOR_WHITE, posGameOver, Alignment::CENTER,  _text);
		small.draw(COLOR_WHITE, posTime, Alignment::CENTER, textScore);

		if(_high) {
			const auto percent = getPulse(_frames, PULSE_TIME, 0);
			const auto pulse = interpolateColor(PULSE_LOW, PULSE_HIGH, percent);
			small.draw(pulse, posBest, Alignment::CENTER, "NEW RECORD!");
		} else {
			const auto score = _selected.getHighScore();
			const auto textBest = std::string("BEST: ") + getTime(static_cast<float>(score));
			small.draw(COLOR_WHITE, posBest, Alignment::CENTER, textBest);
		}

		if(_frames >= FRAMES_PER_GAME_OVER) {
			Buttons a{};
			a.select = true;
			small.draw(COLOR_WHITE, posA, Alignment::CENTER, "PRESS (" + _platform.getButtonName(a) + ") TO PLAY");

			Buttons b{};
			b.back = true;
			small.draw(COLOR_WHITE, posB, Alignment::CENTER, "PRESS (" + _platform.getButtonName(b) + ") TO QUIT");
		}
	}
}
