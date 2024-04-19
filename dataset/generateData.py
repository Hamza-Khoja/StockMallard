import pandas as pd
import numpy as np
import chess
import os
import re
from multiprocessing import Pool

# Global variables to count moves
illegal_moves_count = 0
total_moves_count = 0

def ensure_directory(directory_path):
    """Ensure that the directory exists. If it doesn't, create it."""
    try:
        os.makedirs(directory_path, exist_ok=True)
        print(f"Directory '{directory_path}' ensured.")
    except Exception as e:
        print(f"Failed to create directory '{directory_path}': {e}")

def save_data(filepath, data):
    """Save data to a file, ensuring the directory exists."""
    try:
        directory = os.path.dirname(filepath)
        ensure_directory(directory)
        np.save(filepath, data)
        print(f"Data saved successfully to {filepath}")
    except Exception as e:
        print(f"Error saving data to {filepath}: {e}")

def getBitBoard(board):
    """Converts the given chess board into a bitboard representation."""
    bitboard = []
    for color in [chess.WHITE, chess.BLACK]:
        for piece_type in [chess.PAWN, chess.ROOK, chess.KNIGHT, chess.BISHOP, chess.QUEEN, chess.KING]:
            bitboard.extend([int(board.piece_at(i) == chess.Piece(piece_type, color)) for i in range(64)])
    bitboard.extend([
        int(board.turn),
        int(board.has_kingside_castling_rights(chess.WHITE)),
        int(board.has_queenside_castling_rights(chess.WHITE)),
        int(board.has_kingside_castling_rights(chess.BLACK)),
        int(board.has_queenside_castling_rights(chess.BLACK))
    ])
    return bitboard

def process_game(row):
    global illegal_moves_count, total_moves_count

    board = chess.Board()
    moves = row.get('Moves', '')
    
    moves = re.sub(r'\{[^}]*\}', '', moves)  # Remove comments
    moves = re.sub(r'\[%[^\]]*\]', '', moves)  # Remove annotations
    moves = re.sub(r'[?!]+', '', moves)  # Remove uncertain move markers
    moves = moves.strip().split(' ')  # Split moves into a list

    for move in moves:
        if move.isdigit() or move in ['1-0', '0-1', '1/2-1/2']:
            continue  # Skip standalone move numbers and game results
        try:
            board.push_san(move)
            total_moves_count += 1
        except chess.InvalidMoveError:
            illegal_moves_count += 1
            print(f"Illegal move encountered: {move}")

    return getBitBoard(board)


def process_file(filename):
    """Read and process each game from a CSV file."""
    try:
        df = pd.read_csv(filename)
        print(f"File read successfully: {filename}")
        results = df.apply(process_game, axis=1)
        return results
    except Exception as e:
        print(f"Failed to process file {filename}: {e}")
        return pd.Series()

def main():
    """Main function to orchestrate the multiprocessing of CSV files."""
    global illegal_moves_count, total_moves_count
    try:
        with Pool(processes=2) as pool:
            results_white = pool.apply_async(process_file, ('dataset/CSV_Files/1GB_white_wins.csv',))
            results_black = pool.apply_async(process_file, ('dataset/CSV_Files/1GB_black_wins.csv',))
            
            dataset_white = results_white.get()
            dataset_black = results_black.get()

        save_data('dataset/NPY_Files/white.npy', dataset_white)
        save_data('dataset/NPY_Files/black.npy', dataset_black)

        print(f"Number of illegal moves: {illegal_moves_count}")
        print(f"Total number of moves: {total_moves_count}")
        if total_moves_count > 0:
            print(f"Proportion of illegal moves: {illegal_moves_count / total_moves_count:.2%}")
    except Exception as e:
        print(f"An error occurred in main: {e}")

if __name__ == "__main__":
    main()
