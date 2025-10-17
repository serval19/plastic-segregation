import pandas as pd
import matplotlib.pyplot as plt

# Load your results.csv file
results = pd.read_csv('./trainresults/results.csv')

# Create learning rate curve
plt.figure(figsize=(12, 6))
plt.plot(results['epoch'], results['lr/pg0'], linewidth=2.5, color='blue', label='Learning Rate')
plt.xlabel('Epoch', fontsize=12)
plt.ylabel('Learning Rate', fontsize=12)
plt.title('Learning Rate Schedule During Training', fontsize=14, fontweight='bold')
plt.grid(True, alpha=0.3)
plt.legend()
plt.tight_layout()
plt.savefig('learning_rate_curve.png', dpi=300, bbox_inches='tight')
plt.show()

# Print LR statistics
print(f"Initial Learning Rate: {results['lr/pg0'].iloc[0]:.6f}")
print(f"Final Learning Rate: {results['lr/pg0'].iloc[-1]:.6f}")
print(f"Learning Rate Range: {results['lr/pg0'].min():.6f} - {results['lr/pg0'].max():.6f}")